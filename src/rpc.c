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
#include <limits.h>

#include "rpc.h"
#include "rpc_server.h"
#include "rpc_utils.h"


/**
 * Initialize the server RPC. If NULL is returned, that means the initialization was not
 * successful.
 * @param port the port number
 * @return     NULL if unsuccessful, or the server RPC if otherwise
 */
rpc_server *rpc_init_server(int port) {
    // sockets
    int listen_fd = 0;
    struct addrinfo hints, *results;

    // timeout
    struct timeval timeout = {
        .tv_sec = 5,
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
        fprintf(stderr, "rpc-server: rpc_init_server - "
                        "getaddrinfo unsuccessful\n");
        return NULL;
    }

    // get the appropriate address to create the listen file descriptor
    struct addrinfo *result = results;
    for (; result != NULL; result = result->ai_next) {
        if (result->ai_family == AF_INET6 &&
                (listen_fd = socket(result->ai_family,
                                    result->ai_socktype,
                                    result->ai_protocol
                                    )) >= 0)
            break;
    }
    if (listen_fd < 0) {
        fprintf(stderr, "rpc-server: rpc_init_server - "
                        "listen socket cannot be found\n");
        return NULL;
    }

    // set options to reusable address
    err = setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR,
                     &timeout, sizeof timeout);
    if (err < 0) {
        fprintf(stderr, "rpc-server: rpc_init_server - "
                        "setsockopt unsuccessful\n");
        return NULL;
    }

    // bind to the appropriate address and listen
    err = bind(listen_fd, result->ai_addr, result->ai_addrlen);
    if (err == -1) {
        fprintf(stderr, "rpc-server: rpc_init_server - "
                        "listen socket cannot be bound\n");
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
 * @return        0 if successful, and -1 if otherwise
 */
int rpc_register(rpc_server *server, char *name, rpc_handler handler) {
    // checking if server can register the function or not
    if (server == NULL) {
        fprintf(stderr, "rpc-server: rpc_register - "
                        "server is NULL or its listener is NULL\n");
        return -1;
    }

    // initialize function for registration
    function_t* f = function_init(name, handler);
    if (f == NULL) {
        fprintf(stderr, "rpc-server: rpc_register - "
                        "function_init returns NULL\n");
        return -1;
    }
    return enqueue(server->functions, f);
}


/**
 * Serve the clients - accepting their connections, decompress the payload, and call the
 * function as requested.
 * @param server the server RPC
 */
_Noreturn void rpc_serve_all(rpc_server *server) {
    while (1) {
        // accept connection and update connection socket for server RPC
        struct sockaddr_storage client_addr;
        socklen_t client_addr_size = sizeof client_addr;
        int conn_fd = accept(server->listen_fd,
                             (struct sockaddr *) &client_addr, &client_addr_size);
        if (conn_fd < 0) {
            fprintf(stderr, "rpc-server: rpc_serve_all - "
                            "connect socket cannot accept connections\n");
            continue;
        }
        server->conn_fd = conn_fd;

        // serve rpc_find from client
        function_t* function = rpc_serve_find(server);
        if (function == NULL)
            continue;
        rpc_handler handler = function->f_handler;

        // serve rpc_call from client
        rpc_serve_call(server, handler);
    }
}


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
        fprintf(stderr, "rpc-client: rpc_init_server - "
                        "getaddrinfo unsuccessful\n");
        return NULL;
    }

    // get the appropriate address of server
    struct addrinfo* result = results;
    for (; result != NULL; result = result->ai_next) {
        sock_fd = socket(result->ai_family,
                         result->ai_socktype,
                         result->ai_protocol);
        if (sock_fd == -1)
            continue;
        err = connect(sock_fd, result->ai_addr, result->ai_addrlen);
        if (err != -1)
            break;
        close(sock_fd);
    }
    // no result address found
    if (result == NULL) {
        fprintf(stderr, "rpc-client: rpc_init_client - "
                        "failed to connect\n");
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
    int err;

    // Send the name's length to server
    uint64_t name_len = strlen(name);
    err = rpc_send_uint(client->sock_fd, name_len);
    if (err) {
        fprintf(stderr, "client: rpc_find - "
                        "cannot send function's name to server\n");
        return NULL;
    }

    // Send function name to server
    ssize_t n = send(client->sock_fd, name, name_len, MSG_DONTWAIT);
    if (n < 0) {
        fprintf(stderr, "client: rpc_find - "
                        "cannot send function's name to server\n");
        return NULL;
    }

    // Read the function's id from server
    int64_t id = -1;
    err = rpc_receive_int(client->sock_fd, &id);
    if (err) {
        fprintf(stderr, "client: rpc_find - "
                        "cannot receive function's id from server\n");
        return NULL;
    }

    // check if the function exists or not
    if (id == -1) {
        fprintf(stderr, "client: rpc_find - "
                        "no function with name %s exists on server\n", name);
        return NULL;
    }
    // DEBUG
    printf("client: received function id = %lu\n", id);

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
rpc_data* rpc_call(rpc_client *client, __attribute__((unused)) rpc_handle* handle, rpc_data* payload) {
    // send payload to server
    int err;
    err = rpc_send_payload(client->sock_fd, payload);
    // error messages are implicit in function call
    if (err)
        return NULL;

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
    if (data == NULL) {
        return;
    }
    if (data->data2 != NULL) {
        free(data->data2);
    }
    free(data);
}
