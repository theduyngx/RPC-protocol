/*
 * Author  : The Duy Nguyen - 1100548
 * File    : client_queue.c
 * Purpose : Functions related to the the client queue for server.
 */

#include <stdlib.h>
#include <assert.h>

#include "client_queue.h"
#include "rpc_utils.h"


/* client node data structure */
struct qnode_client {
    int client_fd;
    struct qnode_client* next;
};

/* client queue data structure */
struct queue_client {
    qnode_c* node;
    qnode_c* last;
    int size;
};


/**
 * Initialize a client queue's node.
 * @return  the initialized queue node
 */
qnode_c* client_qnode_init() {
    qnode_c* qn = (qnode_c*) malloc(sizeof(qnode_c));
    assert(qn);
    return qn;
}

/**
 * Initialize a client function queue.
 * @return  the initialized queue
 */
queue_c* client_queue_init() {
    queue_c* q = (queue_c*) malloc(sizeof(queue_c));
    q->node = client_qnode_init();
    q->last = q->node;
    q->size = 0;
    assert(q);
    return q;
}

/**
 * Enqueue a client socket to a given queue.
 * @param  q  the given queue
 * @param  fd the accepted socket to the client
 * @return    0 if enqueued successfully, 1 if not, and -1 if execution fails
 */
int client_enqueue(queue_c* q, int fd) {
    assert(q);
    assert(q->last);
    if (fd < 0) return ERROR;
    q->last->client_fd = fd;
    q->last->next = client_qnode_init();
    q->last = q->last->next;
    (q->size)++;
    return 0;
}

/**
 * Dequeue a client socket from given queue.
 * @param q the given queue
 * @return  the dequeued function
 */
int client_dequeue(queue_c* q) {
    if (q->size <= 0) {
        q->size = 0;
        return ERROR;
    }
    qnode_c* popped = q->node;
    int fd = popped->client_fd;
    q->node = popped->next;
    free(popped);
    (q->size)--;
    assert(q->size >= 0);
    return fd;
}

/**
 * Free memory of given client queue.
 * @param q  the queue
 */
__attribute__((unused))
void free_client_queue(queue_c* q) {
    while (q->size > 0) client_dequeue(q);
    free(q);
}
