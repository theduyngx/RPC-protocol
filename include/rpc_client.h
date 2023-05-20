/*
 * Author  : The Duy Nguyen - 1100548
 * File    : rpc_client.h
 * Purpose : Header for client RPC helper functions and relevant structures.
 */

#ifndef PROJECT2_RPC_CLIENT_H
#define PROJECT2_RPC_CLIENT_H

#include <stdint.h>


/* RPC client structure */
struct rpc_client {
    int conn_fd;
};

/* RPC handle structure */
struct rpc_handle {
    uint64_t function_id;
};

/* function prototypes */
int create_connect_socket(char *addr, int port);

#endif //PROJECT2_RPC_CLIENT_H
