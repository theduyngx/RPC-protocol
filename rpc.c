#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>
#include "rpc.h"
#include "function_queue.h"


struct rpc_server {
    /* Add variable(s) for server state */
    int *conn_fd;
    queue_f* functions;
};


/**
 * Initialize the server RPC. If NULL is returned, that means the initialization was not
 * successful.
 * @param port the port number
 * @return     the server RPC
 */
__attribute__((unused))
rpc_server *rpc_init_server(int port) {
    int listen_fd = 0, conn_fd, re = 1;
    struct addrinfo hints, *results;

    // set all fields in hints to 0, then set specific fields to correspond to IPv6 server
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET6;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    // convert port number and allow all access
    char port_str[10];
    sprintf(port_str, "%d", port);
    int err = getaddrinfo(NULL, port_str, &hints, &results);
    if (err != 0)
        return NULL;

    // get the appropriate address to create the listen file descriptor
    struct addrinfo *result = results;
    for (; result != NULL; result = result->ai_next) {
        if (result->ai_family == AF_INET6 &&
                (listen_fd = socket(result->ai_family,
                                    result->ai_socktype,
                                    result->ai_protocol
                                    )) >= 0)
            break;
        close(listen_fd);
    }

    // set options to reusable address
    setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &re, sizeof re);
    // bind and listen
    err = bind(listen_fd, result->ai_addr, result->ai_addrlen);
    if (err != 0)
        return NULL;
    int listen_queue_size = 10;
    listen(listen_fd, listen_queue_size);

    // accept connection
    struct sockaddr_storage client_addr;
    socklen_t client_addr_size = sizeof client_addr;
    conn_fd = accept(listen_fd, (struct sockaddr*) &client_addr, &client_addr_size);

    // initializing rpc server structure
    struct rpc_server* server = (struct rpc_server*) malloc(sizeof (struct rpc_server));
    server->conn_fd = &conn_fd;
    server->functions = queue_init();
    return server;
}

/**
 * Register a function to the server RPC.
 * @param server  the server RPC
 * @param name    the function's name
 * @param handler the function's handler
 * @return        0 if successful, and -1 if otherwise
 */
__attribute__((unused))
int rpc_register(rpc_server *server, char *name, rpc_handler handler) {
    if (server == NULL || server->conn_fd == NULL)
        return -1;
    function_t* f = function_init(name, handler);
    if (f == NULL)
        return -1;
    enqueue(server->functions, f);
    return 0;
}

__attribute__((unused))
void rpc_serve_all(rpc_server *server) {
}

struct rpc_client {
    /* Add variable(s) for client state */
};

struct rpc_handle {
    /* Add variable(s) for handle */
};

__attribute__((unused))
rpc_client *rpc_init_client(char *addr, int port) {
    return NULL;
}

__attribute__((unused))
rpc_handle *rpc_find(rpc_client *cl, char *name) {
    return NULL;
}

__attribute__((unused))
rpc_data *rpc_call(rpc_client *cl, rpc_handle *h, rpc_data *payload) {
    return NULL;
}

__attribute__((unused))
void rpc_close_client(rpc_client *cl) {

}

__attribute__((unused))
void rpc_data_free(rpc_data *data) {
    if (data == NULL) {
        return;
    }
    if (data->data2 != NULL) {
        free(data->data2);
    }
    free(data);
}
