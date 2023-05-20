/*
 * Author  : The Duy Nguyen - 1100548
 * File    : rpc_server.c
 * Purpose : Server RPC functions specifically related to serving the client's requests.
 *
 * The file also supports multi-threading for server to handle multiple clients at once.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <netdb.h>

#include "rpc_server.h"
#include "rpc_utils.h"


/* ----------------------------- INITIALIZATION ----------------------------- */

/**
 * Create the listen socket for server.
 * @param port        port number
 * @param timeout_sec time (in seconds) before timeout for receive/send occurs
 * @param queue_size  the queue size for accepting clients
 * @return            -1 on failure, and the listen socket on success
 */
int create_listen_socket(int port, int timeout_sec, int queue_size) {
    char* TITLE = "create_listen_socket";
    int err;

    // sockets
    int listen_fd = 0;
    struct addrinfo hints, *results;

    // timeout
    struct timeval timeout = {
            .tv_sec  = timeout_sec,
            .tv_usec = 0
    };

    // set all fields in hints to 0, then set specific fields to correspond to IPv6 server
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET6;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    // convert port number and allow all access
    char port_str[10];
    sprintf(port_str, "%d", port);
    err = getaddrinfo(NULL, port_str, &hints, &results);
    if (err != 0) {
        print_error(TITLE, "getaddrinfo unsuccessful");
        return ERROR;
    }

    // get the appropriate address to create the listen file descriptor
    struct addrinfo *result = results;
    for (; result != NULL; result = result->ai_next) {
        if (result->ai_family == AF_INET6 &&
            (listen_fd = socket(result->ai_family,
                                result->ai_socktype,
                                result->ai_protocol)
            ) >= 0
                ) break;
    }
    if (listen_fd < 0) {
        print_error(TITLE, "listen socket cannot be found");
        return ERROR;
    }

    // set options to reusable address
    int re = 1;
    err  = setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR,
                      &re, sizeof re);
    err += setsockopt(listen_fd, SOL_SOCKET, SO_RCVTIMEO,
                      &timeout, sizeof timeout);
    err += setsockopt(listen_fd, SOL_SOCKET, SO_SNDTIMEO,
                      &timeout, sizeof timeout);
    if (err < 0) {
        print_error(TITLE, "setsockopt unsuccessful");
        return ERROR;
    }

    // bind to the appropriate address and listen
    err = bind(listen_fd, result->ai_addr, result->ai_addrlen);
    if (err < 0) {
        print_error(TITLE, "listen socket cannot be bound");
        return ERROR;
    }
    freeaddrinfo(results);
    listen(listen_fd, queue_size);
    return listen_fd;
}


/* ----------------------------- SERVICE FUNCTIONALITIES ----------------------------- */

/**
 * Server RPC function to serve the find function request from client.
 * @param server  the server RPC
 * @param accept_fd the connection socket to a specific client
 * @return        NULL if no function is found or an error occurs,
 *                or the function structure to serve the call later
 */
function_t* rpc_serve_find(struct rpc_server* server, int accept_fd) {
    char* TITLE = "rpc-server: rpc_serve_all";

    // receive the name's length from client
    int err;
    uint64_t name_len;
    err = rpc_receive_uint(accept_fd, &name_len);
    if (err) {
        print_error(TITLE, "cannot receive the length of client's "
                           "requested function's name");
        return NULL;
    }

    // receive the function's name from client
    char name_buffer[name_len+1];
    memset(name_buffer, 0, name_len+1);
    ssize_t n = recv(accept_fd, name_buffer, name_len, 0);
    if (n < 0) {
        print_error(TITLE, "cannot receive client's requested function name");
        return NULL;
    }
    // Null-terminate string
    assert(name_len == n);
    name_buffer[n] = '\0';

    // check if the function of requested name exists with flag verification
    int flag = ERROR;
    function_t* handler = function_search(server->functions, name_buffer);
    if (handler == NULL)
        print_error(TITLE, "cannot find requested function's name");
    else
        flag = 0;

    // send flag to client, ERROR (or -1) means failure
    err = rpc_send_int(accept_fd, flag);
    if (err) {
        print_error(TITLE, "cannot send function's flag to client");
        return NULL;
    }

    // on success
    if (flag == 0) {
        // finally, we send the function's id to the client
        err = rpc_send_uint(accept_fd, handler->id);
        if (err) {
            print_error(TITLE, "cannot send function's id to client");
            return NULL;
        }
    }
    return handler;
}


/**
 * Server RPC function to serve the call request from client. It will first try to receive
 * from the client the appropriate RPC data packet, and call the handler accordingly.
 * @param server   the server RPC
 * @param accept_fd  the connection socket to a specific client
 * @return         0 if successful, and otherwise if not
 */
int rpc_serve_call(struct rpc_server* server, int accept_fd) {
    char* TITLE = "server: rpc_serve_all";

    // read the function's id to get the function for call
    int err;
    uint64_t id;
    err = rpc_receive_uint(accept_fd, &id);
    if (err) {
        print_error(TITLE, "cannot receive function's id verification from client");
        return ERROR;
    }

    // send verification flag to client
    function_t* function = function_search_id(server->functions, id);
    int flag = -(function == NULL);
    err = rpc_send_int(accept_fd, flag);
    if (err) {
        print_error(TITLE, "cannot send verification flag to client");
        return ERROR;
    }
    if (flag < 0) {
        print_error(TITLE,
                    "verification failed; handle and handler have different ids");
        return ERROR;
    }

    // read the function's payload
    rpc_data* payload = rpc_receive_payload(accept_fd);
    if (payload == NULL)
        return ERROR;

    // call the function
    if (function == NULL || function->f_handler == NULL) {
        rpc_data_free(payload);
        return ERROR;
    }
    rpc_handler handler = function->f_handler;
    rpc_data* response = handler(payload);
    rpc_data_free(payload);

    // send the response to client
    err = rpc_send_payload(accept_fd, response);
    rpc_data_free(response);
    if (err)
        print_error(TITLE, "cannot send the response data to client");
    return err;
}


/* ----------------------------- MULTI-THREADING ----------------------------- */

/* Thread package, which includes needed data
 * to pass in thread package handler
 */
struct thread_package {
    int thread_fd;
    rpc_server* server;
};

void* package_handler(void* package_obj);


/**
 * Initialize the thread package.
 * @param server the server RPC
 */
int package_init(rpc_server* server) {
    // create package
    package_t* package = (package_t*) malloc(sizeof(package_t));
    assert(package && server);
    package->server = server;
    package->thread_fd = server->accept_fd;

    // create thread
    int err;
    pthread_t thread;
    err  = pthread_create(&thread, NULL, package_handler, package);
    err += pthread_detach(thread);
    if (err < 0) {
        free(package);
        pthread_cancel(thread);
        print_error("package_init", "cannot create/detach thread");
    }
    return 0;
}


/**
 * The package handler, which is what each thread will be executing. Specifically,
 * each thread will work to serve a client.
 * @param package_obj the package
 * @return            null pointer with no indication
 */
void* package_handler(void* package_obj) {
    package_t* package = (package_t*) package_obj;
    if (package_obj) {

        // unpacking the package
        rpc_server *server = package->server;
        int thread_fd      = package->thread_fd;
        free(package_obj);
        package_obj = NULL;

        // serve the client
        int flag = ERROR;
        while (rpc_receive_request(thread_fd, &flag) == 0) {
            if      (flag == FIND_SERVICE) rpc_serve_find(server, thread_fd);
            else if (flag == CALL_SERVICE) rpc_serve_call(server, thread_fd);
            else    break;
        }
    }
    return NULL;
}
