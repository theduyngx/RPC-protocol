/*
 * Author  : The Duy Nguyen - 1100548
 * File    : rpc_utils.c
 * Purpose : Utility functions including string hashing, and RPC send/receive data.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <limits.h>
#include <netdb.h>

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
    uint64_t ret_ntw;
    ssize_t n = recv(socket, &ret_ntw, sizeof ret_ntw, MSG_DONTWAIT);
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
    int64_t ret_ntw;
    ssize_t n = recv(socket, &ret_ntw, sizeof ret_ntw, MSG_DONTWAIT);
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
    uint64_t val_ntw = htonl(val);
    ssize_t n = send(socket, &val_ntw, sizeof val_ntw, MSG_DONTWAIT);
    return (n < 0);
}

/**
 * Send a signed integer over the RPC network.
 * @param socket the RPC socket
 * @param ret    the sent value
 * @return       0 if successful, and otherwise if not
 */
int rpc_send_int(int socket, int64_t val) {
    uint64_t val_ntw = htonl(val);
    ssize_t n = send(socket, &val_ntw, sizeof val_ntw, MSG_DONTWAIT);
    return (n < 0);
}


/**
 * Send a payload via a socket to the other end. Naturally, this works for both client and server.
 * @param socket  the specified socket
 * @param payload the specified RPC data payload
 * @return        0 if successful, and otherwise if not
 */
int rpc_send_payload(int socket, rpc_data* payload) {
    // get the relevant properties from payload
    int data1 = payload->data1;
    size_t data2_len = payload->data2_len;

    // send data1
    int err;
    err = rpc_send_int(socket, data1);
    if (err) {
        fprintf(stderr, "rpc-helper: rpc_send_payload - "
                        "cannot send payload's data1 to other end\n");
        return -1;
    }

    // we receive their UINT_MAX
    uint64_t other_max;
    err = rpc_receive_uint(socket, &other_max);
    if (err) {
        fprintf(stderr, "rpc-helper: rpc_send_payload - "
                        "cannot receive other end's UINT_MAX\n");
        return -1;
    }

    // we take the UINT_MAX that is smaller, and take that as a pivot, send to other side
    uint64_t pivot = other_max ? (other_max < UINT_MAX) : UINT_MAX;
    err = rpc_send_uint(socket, pivot);
    if (err) {
        fprintf(stderr, "rpc-helper: rpc_send_payload - "
                        "cannot send this end's UINT_MAX to other end\n");
        return -1;
    }

    // for data2 - due to it being size_t, it may exceed pivot
    size_t data_curr = data2_len;
    int64_t num_exceed = 0;
    while (data_curr > pivot) {
        num_exceed++;
        data_curr -= pivot;
    }

    // first, we send the number of times to send data2_len
    err = rpc_send_int(socket, num_exceed);
    if (err) {
        fprintf(stderr, "rpc-helper: rpc_send_payload - "
                        "cannot send to other end the number of times required to send data2_len\n");
        return -1;
    }

    // then, the remainder
    assert(data_curr < UINT_MAX);
    err = rpc_send_uint(socket, data_curr);
    if (err) {
        fprintf(stderr, "rpc-helper: rpc_send_payload - "
                        "cannot send data2_len remainder to other end\n");
        return -1;
    }

    // receive flag from server, if it is -1, then that means the size of the payload exceeds limit
    int64_t flag;
    err = rpc_receive_int(socket, &flag);
    if (err) {
        fprintf(stderr, "rpc-helper: rpc_send_payload - "
                        "cannot receive other end's file limit flag\n");
        return -1;
    }
    if (flag == -1)
        return -1;

    // if data2_len is not zero, send data2
    if (data2_len > 0) {
        void* data2 = payload->data2;
        // since void type takes up 1 byte, we do not need to do byte ordering
        char buffer[data2_len+1];
        memcpy(buffer, data2, data2_len);
        buffer[data2_len] = '\0';
        ssize_t n = send(socket, buffer, data2_len, MSG_DONTWAIT);
        if (n < 0) {
            fprintf(stderr, "rpc-helper: rpc_send_payload - "
                            "cannot send data2 to other end\n");
            return -1;
        }
    }
    return 0;
}


/**
 * Receive a payload via a socket from the other end. This works for both client and server.
 * @param socket the specified socket
 * @return       the response payload on success, and NULL on failure
 */
rpc_data* rpc_receive_payload(int socket) {
    // receive data1
    int err;
    int64_t data1;
    err = rpc_receive_int(socket, &data1);
    if (err) {
        fprintf(stderr, "rpc-helper: rpc_receive_payload - "
                        "cannot receive payload's data1 from other end\n");
        return NULL;
    }

    // send our UINT_MAX
    err = rpc_send_uint(socket, UINT_MAX);
    if (err) {
        fprintf(stderr, "rpc-helper: rpc_receive_payload - "
                        "cannot send this end's UINT_MAX\n");
        return NULL;
    }

    // receive pivot UINT_MAX
    uint64_t pivot;
    err = rpc_receive_uint(socket, &pivot);
    if (err) {
        fprintf(stderr, "rpc-helper: rpc_receive_payload - "
                        "cannot receive other end's UINT_MAX\n");
        return NULL;
    }

    // receive number of times taken to send data2_len
    int64_t num_send;
    err = rpc_receive_int(socket, &num_send);
    if (err) {
        fprintf(stderr, "rpc-helper: rpc_receive_payload - "
                        "cannot receive from other end the number of times required "
                        "to send data2_len\n");
        return NULL;
    }

    // receive the remainder
    uint64_t remainder;
    err = rpc_receive_uint(socket, &remainder);
    if (err) {
        fprintf(stderr, "rpc-helper: rpc_receive_payload - "
                        "cannot receive data2_len remainder from other end\n");
        return NULL;
    }

    // get this system's SIZE_MAX, viz. the maximum object size possible for the architecture
    // compare that with the pivot INT_MAX * number of times taken + remainder
    // if that number exceeded, send error message, rpc_data size exceeded
    size_t total = pivot * num_send + remainder;
    int64_t flag = 0;
    if (SIZE_MAX < total) flag = -1;
    err = rpc_send_int(socket, flag);
    if (err) {
        fprintf(stderr, "rpc-helper: rpc_receive_payload - "
                        "cannot send limit flag to other end\n");
        return NULL;
    }
    if (flag == -1)
        return NULL;

    // otherwise, receive the package
    char buffer[total+1];
    ssize_t n = recv(socket, buffer, total, MSG_DONTWAIT);
    if (n < 0) {
        fprintf(stderr, "rpc-helper: rpc_receive_payload - "
                        "cannot receive data2 from other end\n");
        return NULL;
    }
    buffer[total] = '\0';
    void* data2 = (void*) malloc(total * sizeof(void));
    assert(total == n);
    memcpy(data2, buffer, total);

    // return the payload
    rpc_data* payload = (rpc_data*) malloc(sizeof(rpc_data));
    /// NOTE: BAD -> type-casting int64 to int
    payload->data1 = (int) data1;
    payload->data2_len = total;
    payload->data2 = data2;
    return payload;
}
