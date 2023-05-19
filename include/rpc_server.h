/*
 * Author  : The Duy Nguyen - 1100548
 * File    : rpc_server.h
 * Purpose : Header file for the server RPC structure, and for server RPC functions
 *           specifically related to serving the client's requests.
 */

#ifndef PROJECT2_RPC_SERVER_H
#define PROJECT2_RPC_SERVER_H

#include <unistd.h>
#include <pthread.h>
#include "function_queue.h"

#define FIND_SERVICE (int) 0
#define CALL_SERVICE (int) 1


/* RPC server structure */
struct rpc_server {
    int listen_fd;
    int conn_fd;
    int num_connections;
    queue_f* functions;
    pthread_t* threads;
};

/* function prototypes to serve clients */
function_t* rpc_serve_find(struct rpc_server* server, int conn_fd);
int rpc_serve_call(struct rpc_server* server, int conn_fd);

/* threading function prototypes */
void rpc_server_threads_init(rpc_server* server);
void new_connection_update(rpc_server* server);
void threads_join(rpc_server* server);

#endif //PROJECT2_RPC_SERVER_H
