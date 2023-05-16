/*
 * File    : server.c
 * Purpose : The server API.
 */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "rpc.h"

/* function prototypes */
rpc_data* add2_i8(rpc_data*);


/**
 * Main entry to the server.
 * @return 0 if execution is successful
 */
int main(int argc, char** argv) {
    rpc_server *state;
    for (int i=0; i < argc; i++) {
        printf("%s\n", argv[i]);
    }

    state = rpc_init_server(3000);
    if (state == NULL) {
        fprintf(stderr, "Failed to init\n");
        exit(EXIT_FAILURE);
    }

    if (rpc_register(state, "add2", add2_i8) == -1) {
        fprintf(stderr, "Failed to register add2\n");
        exit(EXIT_FAILURE);
    }

    rpc_serve_all(state);
    return 0;
}


/**
 * Adds 2 signed 8 bit numbers. ses data1 for left operand, data2 for right operand.
 * @param in  the RPC data input
 * @return    the portable RPC data response
 */
rpc_data* add2_i8(rpc_data* in) {
    /* Check data2 */
    if (in->data2 == NULL || in->data2_len != 1) {
        return NULL;
    }

    /* Parse request */
    char n1 = (char) in->data1;
    char n2 = ((char*) in->data2)[0];

    /* Perform calculation */
    printf("add2: arguments %d and %d\n", n1, n2);
    int res = n1 + n2;

    /* Prepare response */
    rpc_data* out = malloc(sizeof(rpc_data));
    assert(out != NULL);
    out->data1 = res;
    out->data2_len = 0;
    out->data2 = NULL;
    return out;
}
