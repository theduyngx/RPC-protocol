#include <stdlib.h>
#include <netdb.h>
#include <string.h>
#include "rpc.h"
#include "utils.h"


struct rpc_server {
    /* Add variable(s) for server state */
    int *conn_fd;
};

__attribute__((unused))
rpc_server *rpc_init_server(int port) {
    int listen_fd = 0, conn_fd, re = 1;
    struct addrinfo hints, *results;

    // set all fields in hints to 0, then set specific fields to correspond to IPv6 server
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET6;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    // allow all access
    char port_str[10];
    itoa(port, port_str);
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
    struct rpc_server* server = (struct rpc_server*) malloc(sizeof (struct rpc_server));
    server->conn_fd = &conn_fd;
    return server;
}

__attribute__((unused))
int rpc_register(rpc_server *srv, char *name, rpc_handler handler) {
    return -1;
}

__attribute__((unused))
void rpc_serve_all(rpc_server *srv) {
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
