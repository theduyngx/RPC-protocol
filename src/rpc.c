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
#include "rpc_client.h"
#include "rpc_utils.h"

#define NONBLOCKING


/* ------------------------------------- SERVER STUB ------------------------------------- */


/**
 * Initialize the server RPC. If NULL is returned, that means the initialization was not
 * successful.
 * @param port the port number
 * @return     NULL if unsuccessful, or the server RPC if otherwise
 */
rpc_server* rpc_init_server(int port) {
    char* TITLE     = "rpc-server: rpc_init_server";
    int QUEUE_SIZE  = 20;
    int POOL_SIZE   = 20;
    int TIMEOUT_SEC = 5;

    // create listen socket
    int listen_fd = create_listen_socket(port, TIMEOUT_SEC, QUEUE_SIZE);
    if (listen_fd < 0) {
        print_error(TITLE, "rpc-server: cannot create listen socket");
        return NULL;
    }

    // initializing rpc server structure and add its listener
    struct rpc_server* server = (struct rpc_server*) malloc(sizeof (struct rpc_server));
    assert(server);
    server->listen_fd       = listen_fd;
    server->functions       = queue_init();
    server->pool_size       = POOL_SIZE;
    server->num_connections = 0;
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
 * NOTE: There are 2 available methods for using serve all, 1 is thread pool, and the other
 * is simple multi-threaded architecture that takes in as many clients as needed.
 * @param server the server RPC
 */
_Noreturn void rpc_serve_all(rpc_server* server) {
    char* TITLE = "rpc-server: rpc_serve_all";
    int err;
    while (1) {

        // accept connection and update connection socket for server RPC
        struct sockaddr_storage client_addr;
        socklen_t client_addr_size = sizeof client_addr;
        int accept_fd = accept(server->listen_fd,
                               (struct sockaddr *) &client_addr, &client_addr_size);
        if (accept_fd < 0) {
            print_error(TITLE, "connect socket cannot accept connections");
            continue;
        }
        server->accept_fd = accept_fd;

        // enqueue to let the thread handles the connection
        err = package_init(server);
        if (err) print_error(TITLE, "cannot initialize package properly");
    }
}


/* ------------------------------------- CLIENT STUB ------------------------------------- */


/**
 * Initialize the client RPC. If NULL is returned, that means the initialization was not
 * successful.
 * @param addr server's domain address
 * @param port the port number
 * @return     client RPC if successful, or NULL if otherwise
 */
rpc_client* rpc_init_client(char *addr, int port) {
    char* TITLE = "rpc-client: rpc_init_server";
    int conn_fd = create_connect_socket(addr, port);
    if (conn_fd < 0) {
        print_error(TITLE, "cannot create connect socket");
        return NULL;
    }

    // initialize the client RPC
    rpc_client* client = (rpc_client*) malloc(sizeof(rpc_client));
    assert(client);
    client->conn_fd = conn_fd;
    assert(client->conn_fd);
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
    err = rpc_send_int(client->conn_fd, request);
    if (err) {
        print_error(TITLE, "cannot send find service flag to server");
        return NULL;
    }

    // Send the name's length to server
    uint64_t name_len = strlen(name);
    err = rpc_send_uint(client->conn_fd, name_len);
    if (err) {
        print_error(TITLE, "cannot send length of function's name to server");
        return NULL;
    }

    // Send function name to server
    ssize_t n = send(client->conn_fd, name, name_len, 0);
    if (n < 0) {
        print_error(TITLE, "cannot send function's name to server");
        return NULL;
    }

    // Read the function's flag from server ...
    int flag = ERROR;
    err = rpc_receive_int(client->conn_fd, &flag);
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
    err = rpc_receive_uint(client->conn_fd, &id);
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
    err = rpc_send_int(client->conn_fd, request);
    if (err) {
        print_error(TITLE, "cannot send call service flag to server");
        return NULL;
    }

    // send function's id for verification
    err = rpc_send_uint(client->conn_fd, handle->function_id);
    if (err) {
        print_error(TITLE, "cannot send handle to server for verification");
        return NULL;
    }

    // receive the verification flag, if negative then failure
    int flag = ERROR;
    err = rpc_receive_int(client->conn_fd, &flag);
    if (err) {
        print_error(TITLE, "cannot receive verification flag from server");
        return NULL;
    }
    if (flag < 0) {
        print_error(TITLE, "id verification failed");
        return NULL;
    }

    // send payload to server
    err = rpc_send_payload(client->conn_fd, payload);
    if (err) {
        print_error(TITLE, "cannot send payload to server");
        return NULL;
    }

    // receive payload from server
    rpc_data* response = rpc_receive_payload(client->conn_fd);
    return response;
}


/**
 * Cleans up client state and closes client connection.
 * @param client the client RPC
 */
void rpc_close_client(rpc_client *client) {
    close(client->conn_fd);
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
