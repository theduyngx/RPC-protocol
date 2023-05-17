/*
 * Author  : The Duy Nguyen - 1100548
 * File    : rpc_server.c
 * Purpose : Server RPC functions specifically related to serving the client's requests.
 */

#include <stdio.h>
#include <stdlib.h>
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
    // DEBUG
    printf("server's received name length = %lu\n", name_len);

    // receive the function's name from client
    char name_buffer[name_len+1];
    int n = recv(server->conn_fd, name_buffer, name_len, MSG_DONTWAIT);
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
    function_t* handler = search(server->functions, name_buffer);
    uint64_t id;
    if (handler == NULL) {
        fprintf(stderr, "server: rpc_serve_all - "
                        "cannot find requested function's name\n");
        id = -1;
    }
    else id = handler->id;

    // finally, we send the function's id to the client
    err = rpc_send_uint(server->conn_fd, id);
    if (err) {
        fprintf(stderr, "server: rpc_serve_all - "
                        "cannot send function's id to client\n");
        return NULL;
    }
    return handler;
}


/**
 * Server RPC function to serve the call request from client. It will first try to receive
 * from the client the appropriate RPC data packet, and call the handler accordingly.
 * @param server  the server RPC
 * @param handler the RPC function's handler
 * @return        0 if successful, and otherwise if not
 */
int rpc_serve_call(struct rpc_server* server, rpc_handler handler) {
    int n;

    // we must read the function's payload
    n = recv(server->conn_fd, "", 255, MSG_DONTWAIT);
    if (n < 0) {
        fprintf(stderr, "server: rpc_serve_all - cannot read "
                        "required parameters for function call from client\n");
        return -1;
    }

    rpc_data *data = (rpc_data *) malloc(sizeof(rpc_data));
    memcpy(data, "", sizeof "");

    // converting network byte ordering to system's
    data->data1 = ntohs(data->data1);
    printf("data1 = %d, data2_len = %lu", data->data1, data->data2_len);

    // get the required stuff for function somehow here
    // and importantly, send it back to client
    rpc_data *response = handler(data);
    memcpy("", response, sizeof *response);

    /// NOTE: DOES MEMCPY USE STRLEN OR SIZEOF? REMEMBER THAT SIZE_T IS PLATFORM-DEPENDENT
    /// ANOTHER NOTE: STRLEN / SIZEOF THE BUFFER, OR THE RESPONSE?
    /// SHOULDN'T IT BE THE RESPONSE SO THAT THE CLIENT KNOWS WHEN TO STOP READING?
    n = send(server->conn_fd, "", strlen(""), MSG_DONTWAIT);
    if (n < 0) {
        fprintf(stderr, "server: rpc_serve_all - "
                        "cannot send the response to client\n");
        return -1;
    }

    // close the sockets and free structures
    rpc_data_free(data);
    return 0;
}
