/*
 * Author  : The Duy Nguyen - 1100548
 * File    : function_queue.h
 * Purpose : Header file for function queue (the queue which contains all registered function in
 *           the server RPC).
 */

#ifndef PROJECT2_FUNCTION_QUEUE_H
#define PROJECT2_FUNCTION_QUEUE_H

#include <stdint.h>
#include "rpc.h"


/* function data structure */
struct function {
    uint64_t id;
    rpc_handler f_handler;
};
typedef struct function function_t;

/* function structure initialization */
function_t* function_init(char* f_name, rpc_handler f_handler);

/* node data structure */
typedef struct qnode_function qnode_f;

/* queue data structure */
typedef struct queue_function queue_f;

/* queue functions */
queue_f* function_queue_init();
int function_enqueue(queue_f* q, function_t* f);
function_t* function_dequeue(queue_f* q);
function_t* function_search(queue_f* functions, char* name);
function_t* function_search_id(queue_f* functions, uint64_t id);
__attribute__((unused)) void free_function_queue(queue_f* q);

#endif //PROJECT2_FUNCTION_QUEUE_H
