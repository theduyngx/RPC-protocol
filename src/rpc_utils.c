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
#define  DEBUG  0

/*
 * Unsigned integer 64-bit network and system conversion functions. This is used to
 * support the 64-bit integers.
 */
#define htonll(x) ((1 == htonl(1)) ? \
                  (x)              : \
                  ((uint64_t)htonl((x) & 0xFFFFFFFF) << 32) | htonl((x) >> 32))
#define ntohll(x) ((1 == ntohl(1)) ? \
                  (x)              : \
                  ((uint64_t)ntohl((x) & 0xFFFFFFFF) << 32) | ntohl((x) >> 32))


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
 * Debug print error messages.
 * @param title   the function's name and domain
 * @param message the error message
 */
void print_error(__attribute__((unused)) char* title,
                 __attribute__((unused)) char* message) {
    if (DEBUG)
        fprintf(stderr, "%s %s %s\n", title, "-", message);
}


/**
 * Send an unsigned integer 64-bit over the RPC network.
 * @param socket the RPC socket
 * @param ret    the sent value
 * @return       0 if successful, and otherwise if not
 */
int rpc_send_uint(int socket, uint64_t val) {
    uint64_t val_ntw = htonll(val);
    ssize_t n = send(socket, &val_ntw, sizeof val_ntw, 0);
    return (n < 0);
}

/**
 * Receive an unsigned integer 64-bit over the RPC network.
 * @param socket the RPC socket
 * @param ret    the returned value
 * @return       0 if successful, and otherwise if not
 */
int rpc_receive_uint(int socket, uint64_t* ret) {
    uint64_t ret_ntw;
    ssize_t n = recv(socket, &ret_ntw, sizeof ret_ntw, 0);
    if (n < 0) return 1;
    *ret = ntohll(ret_ntw);
    return 0;
}


/**
 * Send a signed integer over the RPC network.
 * @param socket the RPC socket
 * @param ret    the sent value
 * @return       0 if successful, and otherwise if not
 */
int rpc_send_int(int socket, int val) {
    uint64_t val_ntw = htonll((uint64_t) val);
    ssize_t n = send(socket, &val_ntw, sizeof val_ntw, 0);
    return (n < 0);
}

/**
 * Receive a signed integer over the RPC network.
 * @param socket the RPC socket
 * @param ret    the returned value
 * @return       0 if successful, and otherwise if not
 */
int rpc_receive_int(int socket, int* ret) {
    uint64_t ret_ntw;
    ssize_t n = recv(socket, &ret_ntw, sizeof ret_ntw, 0);
    if (n < 0) return 1;
    uint64_t ret64 = ntohll(ret_ntw);

    // negative integer conversion
    if (ret64 >= INT_MAX)
        *ret = -(int) (-ret64);
    else *ret = (int) ret64;
    return 0;
}


/**
 * Send a payload via a socket to the other end. Naturally, this works for both client and server.
 * @param socket  the specified socket
 * @param payload the specified RPC data payload
 * @return        0 if successful, and otherwise if not
 */
int rpc_send_payload(int socket, rpc_data* payload) {
    char* TITLE = "rpc-helper: rpc_send_payload";
    int err;
    int flag;

    // send payload verification flag
    flag = -(payload == NULL);
    err = rpc_send_int(socket, flag);
    if (err) {
        print_error(TITLE, "cannot send payload verification flag to other end");
        return ERROR;
    }
    if (flag != 0) {
        print_error(TITLE, "payload is NULL");
        return ERROR;
    }

    // get the relevant properties from payload
    int data1 = payload->data1;
    size_t data2_len = payload->data2_len;
    void* data2 = payload->data2;

    // send data2 verification flag
    if ((data2_len > 0 && data2 == NULL) || (data2_len == 0 && data2 != NULL))
        flag = -1;
    err = rpc_send_int(socket, flag);
    if (err) {
        print_error(TITLE, "cannot send data2 verification flag to other end");
        return ERROR;
    }
    // verify data2
    if (flag != 0) {
        print_error(TITLE, "data2 is NULL");
        return ERROR;
    }

    // send data1
    err = rpc_send_int(socket, data1);
    if (err) {
        print_error(TITLE, "cannot send payload's data1 to other end");
        return ERROR;
    }

    // we receive their UINT_MAX
    uint64_t other_max;
    err = rpc_receive_uint(socket, &other_max);
    if (err) {
        print_error(TITLE, "cannot receive other end's UINT_MAX");
        return ERROR;
    }
    assert(other_max > 0);

    // we take the UINT_MAX that is smaller, and take that as a pivot, send to other side
    uint64_t pivot;
    if (other_max < UINT_MAX) pivot = other_max;
    else pivot = UINT_MAX;

    err = rpc_send_uint(socket, pivot);
    if (err) {
        print_error(TITLE, "cannot send this end's UINT_MAX to other end");
        return ERROR;
    }

    // for data2 - due to it being size_t, it may exceed pivot
    size_t data_curr = data2_len;
    int num_exceed = 0;
    while (data_curr > pivot) {
        num_exceed++;
        data_curr -= pivot;
    }

    // first, we send the number of times to send data2_len
    err = rpc_send_int(socket, num_exceed);
    if (err) {
        print_error(TITLE,
                    "cannot send to other end the number of times required to send data2_len");
        return ERROR;
    }

    // then, the remainder
    err = rpc_send_uint(socket, data_curr);
    if (err) {
        print_error(TITLE, "cannot send data2_len remainder to other end");
        return ERROR;
    }

    // receive flag from server, if it is ERROR, then that means the size of the payload exceeds limit
    err = rpc_receive_int(socket, &flag);
    if (err) {
        print_error(TITLE, "cannot receive other end's file limit flag");
        return ERROR;
    }
    if (flag == ERROR) {
        print_error(TITLE, "failed verification flag, payload exceeded its limit size");
        return ERROR;
    }

    // send data2 if flag verifies data2 is not NULL
    // since void type takes up 1 byte, we do not need to do byte ordering
    if (data2_len > 0) {
        char buffer[data2_len + 1];
        memcpy(buffer, data2, data2_len);
        buffer[data2_len] = '\0';
        ssize_t n = send(socket, buffer, data2_len, 0);
        if (n < 0) {
            print_error(TITLE, "cannot send data2 to other end");
            return ERROR;
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
    char* TITLE = "rpc-helper: rpc_receive_payload";
    int err;
    int flag;

    // receive payload verification flag
    err = rpc_receive_int(socket, &flag);
    if (err) {
        print_error(TITLE, "cannot receive payload verification flag from other end");
        return NULL;
    }
    if (flag != 0) {
        print_error(TITLE, "payload is NULL");
        return NULL;
    }

    // receive data2 verification flag
    err = rpc_receive_int(socket, &flag);
    if (err) {
        print_error(TITLE,
                    "cannot receive data2 verification flag from other end");
        return NULL;
    }
    // verify data2
    if (flag != 0) {
        print_error(TITLE, "data2 is NULL");
        return NULL;
    }

    // receive data1
    int data1;
    err = rpc_receive_int(socket, &data1);
    if (err) {
        print_error(TITLE, "cannot receive payload's data1 from other end");
        return NULL;
    }

    // send our UINT_MAX
    err = rpc_send_uint(socket, UINT_MAX);
    if (err) {
        print_error(TITLE, "cannot send this end's UINT_MAX");
        return NULL;
    }

    // receive pivot UINT_MAX
    uint64_t pivot;
    err = rpc_receive_uint(socket, &pivot);
    if (err) {
        print_error(TITLE, "cannot receive other end's UINT_MAX");
        return NULL;
    }

    // receive number of times taken to send data2_len
    int num_send;
    err = rpc_receive_int(socket, &num_send);
    if (err) {
        print_error(TITLE, "cannot receive from other end the number of times "
                           "required to send data2_len");
        return NULL;
    }

    // receive the remainder
    uint64_t remainder;
    err = rpc_receive_uint(socket, &remainder);
    if (err) {
        print_error(TITLE, "cannot receive data2_len remainder from other end");
        return NULL;
    }

    // to really compare against SIZE_MAX, we cannot do it directly (looped to 0 if exceeded)
    uint64_t intervals = SIZE_MAX / pivot;
    // so, we compare num_send against number of times taken for pivot to reach SIZE_MAX
    if (intervals < num_send)
        flag = ERROR;
    else if (intervals == num_send) {
        uint64_t interval_remainder = SIZE_MAX % pivot;
        if (interval_remainder <= remainder) flag = ERROR;
    }

    // verify data2 size does not exceed limit
    err = rpc_send_int(socket, flag);
    if (err) {
        print_error(TITLE, "cannot send limit flag to other end");
        return NULL;
    }
    if (flag == ERROR)
        return NULL;

    // otherwise, receive the package
    size_t data2_len = pivot * num_send + remainder;
    void* data2 = NULL;

    // receive data2
    if (data2_len > 0) {
        char buffer[data2_len + 1];
        ssize_t n = recv(socket, buffer, data2_len, 0);
        if (n < 0) {
            print_error(TITLE, "cannot receive data2 from other end");
            return NULL;
        }
        buffer[data2_len] = '\0';
        data2 = (void *) malloc(data2_len * sizeof(void));
        assert(data2_len == n);
        memcpy(data2, buffer, n);
    }

    // return the payload
    rpc_data* payload = (rpc_data*) malloc(sizeof(rpc_data));
    payload->data1 = data1;
    payload->data2_len = data2_len;
    payload->data2 = data2;
    return payload;
}
