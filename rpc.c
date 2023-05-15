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
#include "function_queue.h"


/* RPC server structure */
struct rpc_server {
    int* listen_fd;
    int* conn_fd;
    queue_f* functions;
};


/**
 * Initialize the server RPC. If NULL is returned, that means the initialization was not
 * successful.
 * @param port the port number
 * @return     NULL if unsuccessful, or the server RPC if otherwise
 */
rpc_server *rpc_init_server(int port) {
    int listen_fd = 0, re = 1;
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
    if (err != 0) {
        fprintf(stderr, "rpc_init_server - getaddrinfo unsuccessful\n");
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
        fprintf(stderr, "rpc_init_server - listen socket cannot be found\n");
        return NULL;
    }

    // set options to reusable address
    err = setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR,
                     &re, sizeof re);
    if (err == -1) {
        fprintf(stderr, "rpc_init_server - setsockopt unsuccessful\n");
        return NULL;
    }

    // bind to the appropriate address and listen
    err = bind(listen_fd, result->ai_addr, result->ai_addrlen);
    if (err == -1) {
        fprintf(stderr, "rpc_init_server - listen socket cannot be bound\n");
        return NULL;
    }
    freeaddrinfo(results);
    int listen_queue_size = 10;
    listen(listen_fd, listen_queue_size);

    // initializing rpc server structure and add its listener
    struct rpc_server* server = (struct rpc_server*) malloc(sizeof (struct rpc_server));
    assert(server);
    server->listen_fd = &listen_fd;
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
    if (server == NULL || server->listen_fd == NULL) {
        fprintf(stderr, "rpc_register - server is NULL or its listener is NULL\n");
        return -1;
    }
    // initialize function for registration
    function_t* f = function_init(name, handler);
    if (f == NULL) {
        fprintf(stderr, "rpc_register - function_init returns NULL\n");
        return -1;
    }
    enqueue(server->functions, f);
    return 0;
}


/**
 * Serve the clients - accepting their connections, decompress the payload, and call the
 * function as requested.
 * @param server the server RPC
 */
void rpc_serve_all(rpc_server *server) {
    // accept connection
    struct sockaddr_storage client_addr;
    socklen_t client_addr_size = sizeof client_addr;
    int conn_fd = accept(*(server->listen_fd),
                         (struct sockaddr*) &client_addr, &client_addr_size);
    if (conn_fd == -1) {
        fprintf(stderr, "rpc_serve_all - connect socket cannot accept connections\n");
        return;
    }
    server->conn_fd = &conn_fd;


    /// DEBUGGING:
    char buffer[256];
    // Read characters from the connection, then process
    int n = (int) read(conn_fd, buffer, 255); // n is number of characters read
    if (n < 0) {
        fprintf(stderr, "server: rpc_serve_all - cannot read from client\n");
        exit(EXIT_FAILURE);
    }
    // Null-terminate string
    buffer[n] = '\0';

    // check for name - of course bunch of issues here - buffer size not returned correctly, etc.
    // so this string comparison is really not quite valid
    qnode_f* curr = server->functions->node;
    for (; curr != NULL; curr = curr->next) {
        char* curr_name = curr->function->f_name;
        if (strcmp(curr_name, buffer) == 0)
            break;
    }
    if (curr == NULL)
        fprintf(stderr, "server: rpc_serve_all - "
                        "cannot find requested function's name");

    // Write the data packet back and let client convert that to the RPC handle
    char* some_handle = "This needs to be some handle\n";
    n = (int) write(conn_fd, some_handle, strlen(some_handle));
    if (n < 0) {
        perror("write");
        exit(EXIT_FAILURE);
    }
    rpc_data* data = (rpc_data*) malloc(sizeof(rpc_data));
    memcpy(data, buffer, sizeof buffer);
    printf("data1 = %d, data2_len = %lu", data->data1, data->data2_len);
    rpc_data_free(data);
    ///


    // close the sockets and free structures
    close(*(server->listen_fd));
    close(*(server->conn_fd));
    free_queue(server->functions);
    free(server->listen_fd);
    free(server->conn_fd);
    free(server);
}


/* RPC client structure */
struct rpc_client {
    int* sock_fd;
};

/* RPC handle structure */
struct rpc_handle {
    // ...
};


/**
 * Initialize the client RPC. If NULL is returned, that means the initialization was not
 * successful.
 * @param addr server's domain address
 * @param port the port number
 * @return     client RPC if successful, or NULL if otherwise
 */
rpc_client *rpc_init_client(char *addr, int port) {
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
        fprintf(stderr, "rpc_init_server - getaddrinfo unsuccessful\n");
        return NULL;
    }

    // get the appropriate address of server
    struct addrinfo* result = results;
    for (; result != NULL; result = result->ai_next) {
        sock_fd = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
        if (sock_fd == -1)
            continue;
        err = connect(sock_fd, result->ai_addr, result->ai_addrlen);
        if (err != -1)
            break;
        close(sock_fd);
    }
    // no result address found
    if (result == NULL) {
        fprintf(stderr, "rpc_init_client: failed to connect\n");
        return NULL;
    }
    freeaddrinfo(results);

    // initialize the client RPC
    rpc_client* client = (rpc_client*) malloc(sizeof(rpc_client));
    assert(client);
    client->sock_fd = &sock_fd;
    assert(client->sock_fd);
    return client;
}


/**
 * Find a function from a specific name to get the requirements to make the call.
 * @param client the client RPC
 * @param name   the function's name
 * @return       the requirements to make the call (or the RPC handle), or NULL on error
 */
rpc_handle *rpc_find(rpc_client *client, char *name) {
    /// DEBUG
    char buffer[256];
    int n;

    // Send function name to server
    n = (int) write(*(client->sock_fd), name, strlen(name));
    if (n < 0) {
        fprintf(stderr, "client: rpc_find - cannot write to socket\n");
        return NULL;
    }

    // Read handle from server
    n = (int) read(*(client->sock_fd), buffer, 255);
    if (n < 0) {
        perror("read");
        return NULL;
    }
    // Null-terminate string
    buffer[n] = '\0';
    printf("%s\n", buffer);

    // get the function handle
    rpc_handle* handle = (rpc_handle*) malloc(sizeof(rpc_handle));
    memcpy(handle, buffer, sizeof buffer);

    close(*(client->sock_fd));
    ///
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
rpc_data *rpc_call(rpc_client *client, rpc_handle *handle, rpc_data *payload) {
    return NULL;
}


/**
 * Cleans up client state and closes client connection.
 * @param client the client RPC
 */
void rpc_close_client(rpc_client *client) {
    close(*(client->sock_fd));
    free(client);
}

/**
 * Free the RPC data packet structure.
 * @param data the RPC data
 */
void rpc_data_free(rpc_data *data) {
    if (data == NULL) {
        return;
    }
    if (data->data2 != NULL) {
        free(data->data2);
    }
    free(data);
}
