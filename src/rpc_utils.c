/*
 * Author  : The Duy Nguyen - 1100548
 * File    : rpc_utils.c
 * Purpose : Utility functions including string hashing, and RPC send/receive data.
 */

#include <stdio.h>
#include <netdb.h>
#include <limits.h>

#include "rpc_utils.h"


/**
 * DJB2 hash to hash strings.
 * @param str specified string
 * @return    hashed string value
 */
uint64_t hash(unsigned char* str) {
    uint64_t hash_val = 5381;
    int c;
    while ((c = *str++))
        hash_val = ((hash_val << 5) + hash_val) + c;
    return hash_val;
}


/**
 * Receive an unsigned integer over the RPC network.
 * @param socket the RPC socket
 * @param ret    the returned value
 * @return       0 if successful, and otherwise if not
 */
int rpc_receive_uint(int socket, uint64_t* ret) {
    int n;
    uint64_t ret_ntw;
    n = recv(socket, &ret_ntw, sizeof ret_ntw, MSG_DONTWAIT);
    if (n < 0) return 1;
    *ret = ntohl(ret_ntw);
    return 0;
}

/**
 * Receive a signed integer over the RPC network.
 * @param socket the RPC socket
 * @param ret    the returned value
 * @return       0 if successful, and otherwise if not
 */
int rpc_receive_int(int socket, int64_t* ret) {
    int n;
    int64_t ret_ntw;
    n = recv(socket, &ret_ntw, sizeof ret_ntw, MSG_DONTWAIT);
    if (n < 0) return 1;
    *ret = ntohl(ret_ntw);
    // negative integer conversion
    if (*ret >= INT_MAX)
        *ret = -(int) (- *ret);
    return 0;
}

/**
 * Send an unsigned integer over the RPC network.
 * @param socket the RPC socket
 * @param ret    the sent value
 * @return       0 if successful, and otherwise if not
 */
int rpc_send_uint(int socket, uint64_t val) {
    int n;
    uint64_t val_ntw = htonl(val);
    n = send(socket, &val_ntw, sizeof val_ntw, MSG_DONTWAIT);
    return (n < 0);
}

/**
 * Send a signed integer over the RPC network.
 * @param socket the RPC socket
 * @param ret    the sent value
 * @return       0 if successful, and otherwise if not
 */
int rpc_send_int(int socket, int64_t val) {
    int n;
    uint64_t val_ntw = htonl(val);
    n = send(socket, &val_ntw, sizeof val_ntw, MSG_DONTWAIT);
    return (n < 0);
}
