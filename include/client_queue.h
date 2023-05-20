/*
 * Author  : The Duy Nguyen - 1100548
 * File    : client_queue.h
 * Purpose : Header file for functions related to the the client queue for server.
 */

#ifndef PROJECT2_CLIENT_QUEUE_H
#define PROJECT2_CLIENT_QUEUE_H

#include <stdint.h>
#include "rpc.h"


/* client node data structure */
typedef struct qnode_client qnode_c;

/* client queue data structure */
typedef struct queue_client queue_c;

/* client queue functions */
queue_c* client_queue_init();
int client_enqueue(queue_c* q, int fd);
int client_dequeue(queue_c* q);
__attribute__((unused)) void free_client_queue(queue_c* q);

#endif //PROJECT2_CLIENT_QUEUE_H
