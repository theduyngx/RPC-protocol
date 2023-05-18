/*
 * Author  : The Duy Nguyen - 1100548
 * File    : rpc.c
 * Purpose : The Remote Procedure Call functions for its interface header file.
 *           This entails functions for both server and client side.
 */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <netdb.h>
#include <unistd.h>

#include "rpc.h"
#include "rpc_server.h"
#include "rpc_utils.h"

#define FIND_SERVICE (int) 0
#define CALL_SERVICE (int) 1


/* ------------------------------------- SERVER STUB ------------------------------------- */


/**
 * Initialize the server RPC. If NULL is returned, that means the initialization was not
 * successful.
 * @param port the port number
 * @return     NULL if unsuccessful, or the server RPC if otherwise
 */
rpc_server *rpc_init_server(int port) {
    char* TITLE = "rpc-server: rpc_init_server";

    // sockets
    int listen_fd = 0;
    struct addrinfo hints, *results;

    // timeout
    struct timeval timeout = {
        .tv_sec  = 10,
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
    int err = getaddrinfo(NULL, port_str, &hints, &results);
    if (err != 0) {
        print_error(TITLE, "getaddrinfo unsuccessful");
        return NULL;
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
        return NULL;
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
        return NULL;
    }

    // bind to the appropriate address and listen
    err = bind(listen_fd, result->ai_addr, result->ai_addrlen);
    if (err < 0) {
        print_error(TITLE, "listen socket cannot be bound");
        return NULL;
    }
    freeaddrinfo(results);
    int listen_queue_size = 10;
    listen(listen_fd, listen_queue_size);

    // initializing rpc server structure and add its listener
    struct rpc_server* server = (struct rpc_server*) malloc(sizeof (struct rpc_server));
    assert(server);
    server->listen_fd = listen_fd;
    server->functions = queue_init();
    assert(server->listen_fd && server->functions);
    return server;
}


/**
 * Register a function to the server RPC.
 * @param server  the server RPC
 * @param name    the function's name
 * @param handler the function's handler
 * @return        0 if successful, and ERROR if otherwise
 */
int rpc_register(rpc_server *server, char *name, rpc_handler handler) {
    char* TITLE = "rpc-server: rpc_register";

    // checking if server can register the function or not
    if (server == NULL) {
        print_error(TITLE, "server is NULL or its listener is NULL");
        return ERROR;
    }

    // initialize function for registration
    function_t* f = function_init(name, handler);
    if (f == NULL) {
        print_error(TITLE, "function_init returns NULL");
        return ERROR;
    }
    return enqueue(server->functions, f);
}


/**
 * Serve the clients - accepting their connections, decompress the payload, and call the
 * function as requested.
 * @param server the server RPC
 */
_Noreturn void rpc_serve_all(rpc_server *server) {
    char* TITLE = "rpc-server: rpc_serve_all";

    while (1) {
        // accept connection and update connection socket for server RPC
        struct sockaddr_storage client_addr;
        socklen_t client_addr_size = sizeof client_addr;
        int conn_fd = accept(server->listen_fd,
                             (struct sockaddr *) &client_addr, &client_addr_size);
        if (conn_fd < 0) {
            print_error(TITLE, "connect socket cannot accept connections");
            continue;
        }
        server->conn_fd = conn_fd;

        // receive flag to check which service server must provide
        int flag = ERROR;
        while (rpc_receive_int(server->conn_fd, &flag) == 0) {
            if      (flag == FIND_SERVICE) rpc_serve_find(server);
            else if (flag == CALL_SERVICE) rpc_serve_call(server);
            else    break;
        }
    }
}


/* ------------------------------------- CLIENT STUB ------------------------------------- */


/* RPC client structure */
struct rpc_client {
    int sock_fd;
};

/* RPC handle structure */
struct rpc_handle {
    uint64_t function_id;
};


/**
 * Initialize the client RPC. If NULL is returned, that means the initialization was not
 * successful.
 * @param addr server's domain address
 * @param port the port number
 * @return     client RPC if successful, or NULL if otherwise
 */
rpc_client* rpc_init_client(char *addr, int port) {
    char* TITLE = "rpc-client: rpc_init_server";
    int sock_fd = 0, err;
    struct addrinfo hints, *results;

    // set all fields in hints to 0, then set specific fields to correspond to IPv6 client
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET6;
    hints.ai_socktype = SOCK_STREAM;

    // convert port number and find address of server
    char port_str[10];
    sprintf(port_str, "%d", port);
    err = getaddrinfo(addr, port_str, &hints, &results);
    if (err != 0) {
        print_error(TITLE, "getaddrinfo unsuccessful");
        return NULL;
    }

    // get the appropriate address of server
    struct addrinfo* result = results;
    for (; result != NULL; result = result->ai_next) {
        sock_fd = socket(result->ai_family,
                         result->ai_socktype,
                         result->ai_protocol);
        if (sock_fd < 0)
            continue;
        err = connect(sock_fd, result->ai_addr, result->ai_addrlen);
        if (err != -1)
            break;
        close(sock_fd);
    }
    // no result address found
    if (result == NULL) {
        print_error(TITLE, "failed to connect");
        return NULL;
    }
    freeaddrinfo(results);

    // initialize the client RPC
    rpc_client* client = (rpc_client*) malloc(sizeof(rpc_client));
    assert(client);
    client->sock_fd = sock_fd;
    assert(client->sock_fd);
    return client;
}


/**
 * Find a function from a specific name to get the requirements to make the call.
 * @param client the client RPC
 * @param name   the function's name
 * @return       the requirements to make the call (or the RPC handle), or NULL on error
 */
rpc_handle* rpc_find(rpc_client *client, char *name) {
    char* TITLE = "client: rpc_find";
    int err;

    // send the flag to confirm client is calling find
    int request = FIND_SERVICE;
    err = rpc_send_int(client->sock_fd, request);
    if (err) {
        print_error(TITLE, "cannot send find service flag to server");
        return NULL;
    }

    // Send the name's length to server
    uint64_t name_len = strlen(name);
    err = rpc_send_uint(client->sock_fd, name_len);
    if (err) {
        print_error(TITLE, "cannot send length of function's name to server");
        return NULL;
    }

    // Send function name to server
    ssize_t n = send(client->sock_fd, name, name_len, 0);
    if (n < 0) {
        print_error(TITLE, "cannot send function's name to server");
        return NULL;
    }

    // Read the function's flag from server ...
    int flag = ERROR;
    err = rpc_receive_int(client->sock_fd, &flag);
    if (err) {
        print_error(TITLE, "cannot receive function's flag from server");
        return NULL;
    }

    // check if the function exists or not
    if (flag == ERROR) {
        print_error(TITLE, "no function with name %s exists on server");
        return NULL;
    }

    // Read the function's id from server
    uint64_t id;
    err = rpc_receive_uint(client->sock_fd, &id);
    if (err) {
        print_error(TITLE, "cannot receive function's id from server");
        return NULL;
    }

    // get the function handle
    rpc_handle* handle = (rpc_handle*) malloc(sizeof(rpc_handle));
    handle->function_id = id;
    return handle;
}


/**
 * Run the remote procedure (calling the function as if locally) to obtain the response
 * data from the server. Viz. Calls remote function using handle.
 * @param client  the client RPC
 * @param handle  the RPC handle
 * @param payload the RPC payload (the data to send to server)
 * @return        the response data if successful, or NULL if otherwise
 */
rpc_data* rpc_call(rpc_client *client, rpc_handle* handle, rpc_data* payload) {
    char* TITLE = "rpc-client: rpc_call";
    int err;

    // send the flag to confirm client is calling call
    int request = CALL_SERVICE;
    err = rpc_send_int(client->sock_fd, request);
    if (err) {
        print_error(TITLE, "cannot send call service flag to server");
        return NULL;
    }

    // send function's id for verification
    err = rpc_send_uint(client->sock_fd, handle->function_id);
    if (err) {
        print_error(TITLE, "cannot send handle to server for verification");
        return NULL;
    }

    // receive the verification flag, if negative the failure
    int flag = ERROR;
    err = rpc_receive_int(client->sock_fd, &flag);
    if (err) {
        print_error(TITLE, "cannot receive verification flag from server");
        return NULL;
    }
    if (flag < 0) {
        print_error(TITLE, "id verification failed");
        return NULL;
    }

    // send payload to server
    err = rpc_send_payload(client->sock_fd, payload);
    if (err) {
        print_error(TITLE, "cannot send payload to server");
        return NULL;
    }

    // receive payload from server
    rpc_data* response = rpc_receive_payload(client->sock_fd);
    return response;
}


/**
 * Cleans up client state and closes client connection.
 * @param client the client RPC
 */
void rpc_close_client(rpc_client *client) {
    close(client->sock_fd);
    free(client);
}

/**
 * Free the RPC data packet structure.
 * @param data the RPC data
 */
void rpc_data_free(rpc_data* data) {
    if (data == NULL) return;
    if (data->data2 != NULL)
        free(data->data2);
    free(data);
}
