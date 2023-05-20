/*
 * File    : server.c
 * Purpose : The server API.
*
* The provided API is a simple, bare-bone API which is only to demonstrate how the RPC interface
* functions. Edit the API to however desired, with the RPC interface in mind.
 */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include "rpc.h"

/* function prototypes */
rpc_data* add2_i8(rpc_data*);


/**
 * Main entry to the server.
 * @return 0 if execution is successful
 */
int main(int argc, char** argv) {

    // parse the parameters
    int MIN_ARGC = 3;
    if (argc < MIN_ARGC) {
        fprintf(stderr, "server: insufficient argument\n");
        return 1;
    }
    // flag (so far only port flag is included)
    char* flag = argv[1];
    if (strcmp(flag, "-p") != 0) {
        fprintf(stderr, "server: invalid flag\n");
        return 1;
    }
    // port number - probably should use strtol()
    char* port_str = argv[2];
    int port = atoi(port_str); // NOLINT(cert-err34-c)

    // initializing the server RPC with port number
    rpc_server *state;
    state = rpc_init_server(port);
    if (state == NULL) {
        fprintf(stderr, "Failed to init\n");
        exit(EXIT_FAILURE);
    }

    // register functions to server RPC
    // NOTE: this needs to be converted to full-fledged IO stuff too
    if (rpc_register(state, "add2", add2_i8) == -1) {
        fprintf(stderr, "Failed to register add2\n");
        exit(EXIT_FAILURE);
    }

    // serve the clients
    rpc_serve_all(state);
}


/**
 * Adds 2 signed 8 bit numbers. ses data1 for left operand, data2 for right operand.
 * @param in  the RPC data input
 * @return    the portable RPC data response
 */
rpc_data* add2_i8(rpc_data* in) {
    // check data2
    if (in->data2 == NULL || in->data2_len != 1)
        return NULL;

    // Parse request
    char n1 = (char) in->data1;
    char n2 = ((char*) in->data2)[0];

    // Perform calculation
    printf("add2: arguments %d and %d\n", n1, n2);
    int res = n1 + n2;

    // Prepare response
    rpc_data* out = malloc(sizeof(rpc_data));
    assert(out != NULL);
    out->data1 = res;
    out->data2_len = 0;
    out->data2 = NULL;
    return out;
}
