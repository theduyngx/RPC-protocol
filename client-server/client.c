/*
 * File    : client.c
 * Purpose : The client API.
 */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <getopt.h>

#include "rpc.h"


/**
 * Main entry to client program.
 * @return 0 if successful
 */
int main(int argc, char** argv) {

    // parse the arguments
    int MIN_ARGC = 4;
    if (argc < MIN_ARGC) {
        fprintf(stderr, "client: insufficient arguments\n");
        exit(EXIT_FAILURE);
    }
    // flags
    char* ip   = NULL;
    char* port = NULL;
    int c;

    // check input flags
    while ((c = getopt(argc, argv, "i:p:")) != -1) {
        switch (c) {
            case 'i':
                ip = optarg;
                break;
            case 'p':
                port = optarg;
                break;
            case '?':
                fprintf(stderr, "client: error in option -%c. Aborting...\n", optopt);
            default:
                exit(1);
        }
    }
    // check if any flag is left empty
    if (ip == NULL) {
        fprintf(stderr, "client: no -i flag for IP address found\n");
        exit(EXIT_FAILURE);
    }
    if (port == NULL) {
        fprintf(stderr, "client: no -p flag for port number found\n");
        exit(EXIT_FAILURE);
    }

    // convert port to integer to initialize client RPC
    int port_num = atoi(port); // NOLINT(cert-err34-c)
    int exit_code = 0;
    rpc_client* state = rpc_init_client(ip, port_num);
    if (state == NULL)
        exit(EXIT_FAILURE);

    // find function on server
    rpc_handle* handle_add2 = rpc_find(state, "add2");
    if (handle_add2 == NULL) {
        fprintf(stderr, "client: function add2 does not exist\n");
        exit_code = 1;
        goto cleanup;
    }

    for (int i = 0; i < 2; i++) {
        // Prepare request
        char left_operand = (char) i;
        char right_operand = 100;

        // IO operation to fetch request here?
        rpc_data request_data = {
            .data1 = left_operand,
            .data2_len = 1, // ???
            .data2 = &right_operand
        };

        // Call and receive response
        rpc_data* response_data = rpc_call(state, handle_add2, &request_data);
        if (response_data == NULL) {
            fprintf(stderr, "Function call of add2 failed\n");
            exit_code = 1;
            goto cleanup;
        }

        // Interpret response
        assert(response_data->data2_len == 0);
        assert(response_data->data2 == NULL);
        printf("Result of adding %d and %d: %d\n", left_operand, right_operand,
               response_data->data1);
        rpc_data_free(response_data);
    }

    // cleanup handle and RPC
cleanup:
    if (handle_add2 != NULL)
        free(handle_add2);
    rpc_close_client(state);
    state = NULL;
    return exit_code;
}
