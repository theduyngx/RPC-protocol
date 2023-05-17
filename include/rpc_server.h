/*
 * Author  : The Duy Nguyen - 1100548
 * File    : rpc_server.h
 * Purpose : Header file for the server RPC structure, and for server RPC functions
 *           specifically related to serving the client's requests.
 */

#ifndef PROJECT2_RPC_SERVER_H
#define PROJECT2_RPC_SERVER_H

#include "function_queue.h"

/* RPC server structure */
struct rpc_server {
    int listen_fd;
    int conn_fd;
    queue_f* functions;
};

/* function prototypes to serve clients */
function_t* rpc_serve_find(struct rpc_server* server);
int rpc_serve_call(struct rpc_server* server, rpc_handler handler);

#endif //PROJECT2_RPC_SERVER_H
