/*
 * Author  : The Duy Nguyen - 1100548
 * File    : rpc_server.c
 * Purpose : Server RPC functions specifically related to serving the client's requests.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <netdb.h>

#include "rpc_server.h"
#include "rpc_utils.h"


/* ----------------------------- SERVICE FUNCTIONALITIES ----------------------------- */

/**
 * Server RPC function to serve the find function request from client.
 * @param server  the server RPC
 * @param conn_fd the connection socket to a specific client
 * @return        NULL if no function is found or an error occurs,
 *                or the function structure to serve the call later
 */
function_t* rpc_serve_find(struct rpc_server* server, int conn_fd) {
    char* TITLE = "rpc-server: rpc_serve_all";

    // receive the name's length from client
    int err;
    uint64_t name_len;
    err = rpc_receive_uint(conn_fd, &name_len);
    if (err) {
        print_error(TITLE, "cannot receive the length of client's "
                           "requested function's name");
        return NULL;
    }

    // receive the function's name from client
    char name_buffer[name_len+1];
    memset(name_buffer, 0, name_len+1);
    ssize_t n = recv(conn_fd, name_buffer, name_len, 0);
    if (n < 0) {
        print_error(TITLE, "cannot receive client's requested function name");
        return NULL;
    }
    // Null-terminate string
    assert(name_len == n);
    name_buffer[n] = '\0';

    // check if the function of requested name exists with flag verification
    int flag = ERROR;
    function_t* handler = search(server->functions, name_buffer);
    if (handler == NULL)
        print_error(TITLE, "cannot find requested function's name");
    else
        flag = 0;

    // send flag to client, ERROR (or -1) means failure
    err = rpc_send_int(conn_fd, flag);
    if (err) {
        print_error(TITLE, "cannot send function's flag to client");
        return NULL;
    }

    // on success
    if (flag == 0) {
        // finally, we send the function's id to the client
        err = rpc_send_uint(conn_fd, handler->id);
        if (err) {
            print_error(TITLE, "cannot send function's id to client");
            return NULL;
        }
    }
    return handler;
}


/**
 * Server RPC function to serve the call request from client. It will first try to receive
 * from the client the appropriate RPC data packet, and call the handler accordingly.
 * @param server   the server RPC
 * @param conn_fd  the connection socket to a specific client
 * @return         0 if successful, and otherwise if not
 */
int rpc_serve_call(struct rpc_server* server, int conn_fd) {
    char* TITLE = "server: rpc_serve_all";

    // read the function's id to get the function for call
    int err;
    uint64_t id;
    err = rpc_receive_uint(conn_fd, &id);
    if (err) {
        print_error(TITLE, "cannot receive function's id verification from client");
        return ERROR;
    }

    // send verification flag to client
    function_t* function = search_id(server->functions, id);
    int flag = -(function == NULL);
    err = rpc_send_int(conn_fd, flag);
    if (err) {
        print_error(TITLE, "cannot send verification flag to client");
        return ERROR;
    }
    if (flag < 0) {
        print_error(TITLE,
                    "verification failed; handle and handler have different ids");
        return ERROR;
    }

    // read the function's payload
    rpc_data* payload = rpc_receive_payload(conn_fd);
    if (payload == NULL)
        return ERROR;

    // call the function
    if (function == NULL || function->f_handler == NULL) {
        rpc_data_free(payload);
        return ERROR;
    }
    rpc_handler handler = function->f_handler;
    rpc_data* response = handler(payload);
    rpc_data_free(payload);

    // send the response to client
    err = rpc_send_payload(conn_fd, response);
    rpc_data_free(response);
    if (err)
        print_error(TITLE, "cannot send the response data to client");
    return err;
}


/* ----------------------------- THREADING FUNCTIONALITIES ----------------------------- */

/* Mutex and thread condition */
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t condition = PTHREAD_COND_INITIALIZER;

_Noreturn void* thread_serve(void* obj);


/**
 * Initialize the server's threads.
 * @param server the server RPC
 */
void rpc_server_threads_init(rpc_server* server) {
    int pool_size = server->pool_size;
    server->threads = (pthread_t*) malloc(pool_size * sizeof(pthread_t));
    for (int i=0; i < pool_size; i++) {
        pthread_create(&(server->threads[i]), NULL,
                       thread_serve, server);
    }
}


/**
 * Thread handler function. This will make the thread check if there are any clients
 * un-served and will thus serve them if resources are available. It uses a mutex lock
 * and a conditional variable to ensure efficient resource usage.
 * @param server_obj the server
 * @return           nothing - the threads are expected to run forever
 */
_Noreturn void* thread_serve(void* server_obj) {
    rpc_server* server = (rpc_server*) server_obj;
    while (1) {
        int thread_fd = -1;

        // check if any client is not yet served
        pthread_mutex_lock(&mutex);
        pthread_cond_wait(&condition, &mutex);
        int cond = (server->num_connections > 0);
        if (cond) {
            (server->num_connections)--;
            thread_fd = server->conn_fd;
        }
        pthread_mutex_unlock(&mutex);

        // serve the client
        if (cond) {
            int flag = ERROR;
            while (rpc_receive_int(thread_fd, &flag) == 0) {
                if      (flag == FIND_SERVICE) rpc_serve_find(server, thread_fd);
                else if (flag == CALL_SERVICE) rpc_serve_call(server, thread_fd);
                else    break;
            }
        }
    }
}

/**
 * Critical region to update a new connection from server, viz. when a new client
 * has successfully connected to server.
 * @param server
 */
void new_connection_update(rpc_server* server) {
    pthread_mutex_lock(&mutex);
    (server->num_connections)++;
    pthread_cond_signal(&condition);
    pthread_mutex_unlock(&mutex);
}


/**
 * Joining server's threads. This will be called periodically.
 * @param server
 */
void threads_join(rpc_server* server) {
    for (int i=0; i < server->num_connections; i++) {
        pthread_t thread = server->threads[i];
        pthread_join(thread, NULL);
    }
}
