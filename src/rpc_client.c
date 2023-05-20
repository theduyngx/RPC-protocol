/*
 * Author  : The Duy Nguyen - 1100548
 * File    : rpc_client.c
 * Purpose : Client RPC helper functions.
 */

#include <stdio.h>
#include <string.h>
#include <netdb.h>
#include <unistd.h>

#include "rpc_utils.h"


/**
 * Create the connect socket for client.
 * @param addr the address name to connect to (the IP address)
 * @param port the port number of the server
 * @return     the connect socket
 */
int create_connect_socket(char *addr, int port) {
    char* TITLE = "create_connect_socket";
    int err;

    // connect socket and address information to connect to
    int conn_fd = ERROR;
    struct addrinfo hints, *results;

    // set all fields in hints to 0, then set specific fields to correspond to IPv6 client
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET6;
    hints.ai_socktype = SOCK_STREAM;

    // convert port number and find address of server
    char port_str[10];
    sprintf(port_str, "%d", port);
    err = getaddrinfo(addr, port_str, &hints, &results);
    if (err != 0) {
        print_error(TITLE, "getaddrinfo unsuccessful");
        goto cleanup;
    }

    // get the appropriate address of server
    struct addrinfo* result = results;
    for (; result != NULL; result = result->ai_next) {
        conn_fd = socket(result->ai_family,
                         result->ai_socktype,
                         result->ai_protocol);
        if (conn_fd < 0) continue;
        err = connect(conn_fd, result->ai_addr, result->ai_addrlen);
        if (err != -1) break;
        close(conn_fd);
    }
    // no result address found
    if (result == NULL) {
        print_error(TITLE, "failed to connect");
        goto cleanup;
    }

    // cleanup and return the socket
    cleanup:
    freeaddrinfo(results);
    return conn_fd;
}