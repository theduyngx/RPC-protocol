/*
 * Author  : The Duy Nguyen - 1100548
 * File    : rpc_utils.c
 * Purpose : Header for utility functions including string hashing,
 *           and RPC send/receive data.
 */

#ifndef PROJECT2_RPC_UTILS_H
#define PROJECT2_RPC_UTILS_H

#include <stdint.h>

uint64_t hash(unsigned char* str);
int rpc_receive_uint(int socket, uint64_t* ret);
int rpc_receive_int(int socket, int64_t* ret);
int rpc_send_uint(int socket, uint64_t val);
int rpc_send_int(int socket, int64_t val);

#endif //PROJECT2_RPC_UTILS_H
