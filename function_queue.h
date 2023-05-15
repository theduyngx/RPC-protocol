/*
 * Author  : The Duy Nguyen - 1100548
 * File    : input_queue.h
 * Purpose : Header file for input queue (the queue which contains all processes that request to
 *           be run). The input queue is implemented using the queue data structure.
 */

#ifndef PROJECT1_CLION_UTILS_H
#define PROJECT1_CLION_UTILS_H

#include "rpc.h"

#define MIN_NAME_LEN 3


/* function */
struct function {
    char* f_name;
    rpc_handler f_handler;
};
typedef struct function function_t;

function_t* function_init(char* f_name, rpc_handler f_handler);

/* node data structure */
struct qnode {
    function_t* function;
    struct qnode* next;
};
typedef struct qnode qnode_f;

/* queue data structure */
struct queue {
    qnode_f* node;
    qnode_f* last;
    int size;
};
typedef struct queue queue_f;

/* queue functions */
queue_f* queue_init();
void enqueue(queue_f* q, function_t* f);
function_t* dequeue(queue_f* q);
void free_queue(queue_f* q);

#endif //PROJECT1_CLION_UTILS_H
