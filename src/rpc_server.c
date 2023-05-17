/*
 * Author  : The Duy Nguyen - 1100548
 * File    : rpc_server.c
 * Purpose : Server RPC functions specifically related to serving the client's requests.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include "rpc_server.h"


/**
 * Server RPC function to serve the find function request from client.
 * @param server the server RPC
 * @return       NULL if no function is found or an error occurs,
 *               or the function structure to serve the call later
 */
function_t* rpc_serve_find(struct rpc_server* server) {
    // receive the function's name from client
    char name_buffer[256];
    int n = recv(server->conn_fd, name_buffer, 255, MSG_DONTWAIT);
    if (n < 0) {
        fprintf(stderr, "rpc-server: rpc_serve_all - cannot read from client\n");
        return NULL;
    }
    // Null-terminate string
    name_buffer[n] = '\0';

    // check for name - of course bunch of issues here - buffer size not returned correctly, etc.
    // so this string comparison is really not quite valid
    qnode_f *curr = server->functions->node;
    for (; curr != NULL; curr = curr->next) {
        char *curr_name = curr->function->f_name;
        if (strncmp(curr_name, name_buffer, n) == 0)
            break;
    }
    if (curr == NULL)
        fprintf(stderr, "server: rpc_serve_all - "
                        "cannot find requested function's name\n");

    // get the function handler id corresponding to the request
    function_t* handler = curr->function;
    uint64_t id = curr->function->id;

    // first we send the function's id to the client
    n = send(server->conn_fd, &id, sizeof id, MSG_DONTWAIT);
    if (n < 0) {
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
 * @return
 */
int rpc_serve_call(struct rpc_server* server, rpc_handler handler) {
    int n;

    // after that, we must read, or receive from client's again, this is when client calls rpc_call
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
