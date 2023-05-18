/*
 * Author  : The Duy Nguyen - 1100548
 * File    : rpc_utils.c
 * Purpose : Header for utility functions including string hashing,
 *           and RPC send/receive data.
 */

#ifndef PROJECT2_RPC_UTILS_H
#define PROJECT2_RPC_UTILS_H

#include <stdint.h>
#include "rpc.h"

#define ERROR (int) (-1)


/* hash and debug */
uint64_t hash(unsigned char* str);
void print_error(char* title, char* message);

/* send/receive unsigned integer 64-bit */
int rpc_send_uint(int socket, uint64_t val);
int rpc_receive_uint(int socket, uint64_t* ret);

/* send/receive signed integer */
int rpc_send_int(int socket, int val);
int rpc_receive_int(int socket, int* ret);

/* send/receive rpc data */
int rpc_send_payload(int socket, rpc_data* payload);
rpc_data* rpc_receive_payload(int socket);

#endif //PROJECT2_RPC_UTILS_H
