/*
 * Author  : The Duy Nguyen - 1100548
 * File    : rpc_server.h
 * Purpose : Header file for the server RPC structure, and for server RPC functions
 *           specifically related to serving the client's requests.
 */

#ifndef PROJECT2_RPC_SERVER_H
#define PROJECT2_RPC_SERVER_H

#include <pthread.h>
#include "function_queue.h"

#define FIND_SERVICE (int) 0    // flag from client requesting find service
#define CALL_SERVICE (int) 1    // flag from client requesting call service


/* RPC server structure */
struct rpc_server {
    int listen_fd;
    int accept_fd;
    queue_f* functions;
};

/* listen socket creation */
int create_listen_socket(int port, int timeout_sec, int queue_size);

/* function prototypes to serve clients */
function_t* rpc_serve_find(struct rpc_server* server, int conn_fd);
int rpc_serve_call(struct rpc_server* server, int conn_fd);


/* Thread package */
typedef struct thread_package package_t;

/* simple multi-threading function prototypes */
__attribute__((unused))
int package_init(rpc_server* server);

#endif //PROJECT2_RPC_SERVER_H
