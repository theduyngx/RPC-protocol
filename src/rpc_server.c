/*
 * Author  : The Duy Nguyen - 1100548
 * File    : rpc_server.c
 * Purpose : Server RPC functions specifically related to serving the client's requests.
 */

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <netdb.h>

#include "rpc_server.h"
#include "rpc_utils.h"


/**
 * Server RPC function to serve the find function request from client.
 * @param server the server RPC
 * @return       NULL if no function is found or an error occurs,
 *               or the function structure to serve the call later
 */
function_t* rpc_serve_find(struct rpc_server* server) {

    // receive the name's length from client
    int err;
    uint64_t name_len;
    err = rpc_receive_uint(server->conn_fd, &name_len);
    if (err) {
        fprintf(stderr, "rpc-server: rpc_serve_all - "
                        "cannot receive the length of client's requested function's name\n");
        return NULL;
    }
    /// DEBUG
    printf("server's received name length = %lu\n", name_len);
    ///

    // receive the function's name from client
    char name_buffer[name_len+1];
    ssize_t n = recv(server->conn_fd, name_buffer, name_len, 0);
    if (n < 0) {
        fprintf(stderr, "rpc-server: rpc_serve_all - "
                        "cannot receive client's requested function name\n");
        return NULL;
    }
    // Null-terminate string
    assert(name_len == n);
    name_buffer[n] = '\0';

    // check if the function of requested name exists
    // do note that id will be -1 if no handler is found
    int flag = -1;
    function_t* handler = search(server->functions, name_buffer);
    if (handler == NULL)
        fprintf(stderr, "server: rpc_serve_all - "
                        "cannot find requested function's name\n");
    else
        flag = 0;

    // send flag to client, -1 means failure
    err = rpc_send_int(server->conn_fd, flag);
    if (err) {
        fprintf(stderr, "server: rpc_serve_all - "
                        "cannot send function's flag to client\n");
        return NULL;
    }

    // on success
    if (flag == 0) {
        // finally, we send the function's id to the client
        ///
        printf("\n");
        printf("RPC-SERVER: THE FOUND ID IS %lu\n", handler->id);
        printf("\n");
        ///

        err = rpc_send_uint(server->conn_fd, handler->id);
        if (err) {
            fprintf(stderr, "server: rpc_serve_all - "
                            "cannot send function's id to client\n");
            return NULL;
        }
    }

    return handler;
}


/**
 * Server RPC function to serve the call request from client. It will first try to receive
 * from the client the appropriate RPC data packet, and call the handler accordingly.
 * @param server   the server RPC
 * @param function the RPC function
 * @return         0 if successful, and otherwise if not
 */
int rpc_serve_call(struct rpc_server* server, function_t* function) {
    // read the function's id to verify the id
    int err;
    uint64_t id;
    err = rpc_receive_uint(server->conn_fd, &id);
    if (err) {
        fprintf(stderr, "server: rpc_serve_all - "
                        "cannot receive function's id verification from client\n");
        return -1;
    }

    // send verification flag to client
    int flag = -(id != function->id);
    err = rpc_send_int(server->conn_fd, flag);
    if (err) {
        fprintf(stderr, "server: rpc_serve_all - "
                        "cannot send verification flag to client\n");
        return -1;
    }
    if (flag < 0) {
        fprintf(stderr, "server: rpc_serve_all - "
                        "verification failed; handle and handler have different ids\n");
        printf("%lu %lu\n", id, function->id);
        return -1;
    }

    // read the function's payload
    rpc_data* payload = rpc_receive_payload(server->conn_fd);
    if (payload == NULL) {
        err = -1;
        goto cleanup;
    }

    // call the function
    rpc_handler handler = function->f_handler;
    rpc_data* response = handler(payload);

    // send the response to client
    err = rpc_send_payload(server->conn_fd, response);
    if (err)
        fprintf(stderr, "server: rpc_serve_all - "
                        "cannot send the response data to client\n");

    // close the sockets and free structures
    rpc_data_free(response);
    cleanup:
    rpc_data_free(payload);
    return err;
}
