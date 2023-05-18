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
    char* TITLE = "rpc-server: rpc_serve_all";

    // receive the name's length from client
    int err;
    uint64_t name_len;
    err = rpc_receive_uint(server->conn_fd, &name_len);
    if (err) {
        print_error(TITLE, "cannot receive the length of client's "
                           "requested function's name");
        return NULL;
    }

    // receive the function's name from client
    char name_buffer[name_len+1];
    ssize_t n = recv(server->conn_fd, name_buffer, name_len, 0);
    if (n < 0) {
        print_error(TITLE, "cannot receive client's requested function name");
        return NULL;
    }
    // Null-terminate string
    assert(name_len == n);
    name_buffer[n] = '\0';

    // check if the function of requested name exists with flag verification
    int flag = ERROR;
    function_t* handler = search(server->functions, name_buffer);
    if (handler == NULL)
        print_error(TITLE, "cannot find requested function's name");
    else
        flag = 0;

    // send flag to client, ERROR (or -1) means failure
    err = rpc_send_int(server->conn_fd, flag);
    if (err) {
        print_error(TITLE, "cannot send function's flag to client");
        return NULL;
    }

    // on success
    if (flag == 0) {
        // finally, we send the function's id to the client
        err = rpc_send_uint(server->conn_fd, handler->id);
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
 * @return         0 if successful, and otherwise if not
 */
int rpc_serve_call(struct rpc_server* server) {
    char* TITLE = "server: rpc_serve_all";

    // read the function's id to get the function for call
    int err;
    uint64_t id;
    err = rpc_receive_uint(server->conn_fd, &id);
    if (err) {
        print_error(TITLE, "cannot receive function's id verification from client");
        return ERROR;
    }

    // send verification flag to client
    function_t* function = search_id(server->functions, id);
    int flag = -(function == NULL);
    err = rpc_send_int(server->conn_fd, flag);
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
    rpc_data* payload = rpc_receive_payload(server->conn_fd);
    if (payload == NULL)
        return ERROR;

    // call the function
    if (function == NULL || function->f_handler == NULL)
        return ERROR;
    rpc_handler handler = function->f_handler;
    rpc_data* response = handler(payload);

    // send the response to client
    err = rpc_send_payload(server->conn_fd, response);

    ///
//    printf("\n");
//    printf("data1 = %d\n", response->data1);
//    printf("data2_len = %lu\n", response->data2_len);
//    printf("data2 = %p\n", response->data2);
//    printf("\n");
    ///

    if (err)
        print_error(TITLE, "cannot send the response data to client");
    return err;
}
