/*
 * Author  : The Duy Nguyen - 1100548
 * File    : function_queue.c
 * Purpose : Functions related to the function structure and queue structure for storing said
 *           function structure in RPC.
 *
 * The function queue holds different, unique functions with no duplicates. This will be checked
 * whenever adding another function to the queue.
 */

#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "function_queue.h"
#include "rpc_utils.h"


/* node data structure */
struct qnode_function {
    function_t* function;
    struct qnode_function* next;
};

/* queue data structure */
struct queue_function {
    qnode_f* node;
    qnode_f* last;
    int size;
};


/**
 * Initialize a function.
 * @param f_name    the function's name
 * @param f_handler the function's handler
 * @return          the function
 */
function_t* function_init(char* f_name, rpc_handler f_handler) {
    char* TITLE = "function_init";

    uint64_t name_len = strlen(f_name);
    if (name_len == 0) {
        print_error(TITLE, "name length = 0");
        return NULL;
    }
    for (int i=0; i < strlen(f_name); i++) {
        char character = f_name[i];
        int ascii = (int) character;
        if (ascii < 32 || ascii > 126) {
            print_error(TITLE, "ascii character not within range 32 and 126");
            return NULL;
        }
    }
    function_t* f = (function_t*) malloc(sizeof(function_t));
    assert(f);
    uint64_t id = hash((unsigned char*) f_name);
    f->id = id;
    f->f_handler = f_handler;
    return f;
}


/**
 * Initialize a queue's node.
 * @return  the initialized queue node
 */
qnode_f* function_qnode_init() {
    qnode_f* qn = (qnode_f*) malloc(sizeof(qnode_f));
    assert(qn);
    return qn;
}

/**
 * Initialize a function queue.
 * @return  the initialized queue
 */
queue_f* function_queue_init() {
    queue_f* q = (queue_f*) malloc(sizeof(queue_f));
    q->node = function_qnode_init();
    q->last = q->node;
    q->size = 0;
    assert(q);
    return q;
}

/**
 * Enqueue a process to a given queue.
 * @param  q  the given queue
 * @param  f  the function to be enqueued
 * @return    0 if enqueued successfully, 1 if not, and -1 if execution fails
 */
int function_enqueue(queue_f* q, function_t* f) {
    assert(q);
    assert(q->last);
    if (! f) return -1;

    // ensure uniqueness in registration
    qnode_f* curr = q->node;
    for (int i=0; i < q->size; i++) {
        function_t* f_curr = curr->function;
        if (f_curr->id == f->id)
            return 1;
        curr = curr->next;
    }
    q->last->function = f;
    q->last->next = function_qnode_init();
    q->last = q->last->next;
    (q->size)++;
    return 0;
}

/**
 * Dequeue a process from given queue.
 * @param q the given queue
 * @return  the dequeued function
 */
function_t* function_dequeue(queue_f* q) {
    if (q->size <= 0) {
        q->size = 0;
        return NULL;
    }
    qnode_f* popped = q->node;
    function_t* f = popped->function;
    q->node = popped->next;
    free(popped);
    (q->size)--;
    assert(q->size >= 0);
    return f;
}

/**
 * Search for a function with the specified function name.
 * @param functions function queue
 * @param name      the specified function's name
 * @return          NULL if no function of name found,
 *                  or the function structure if found
 */
function_t* function_search(queue_f* functions, char* name) {
    uint64_t hashed = hash((unsigned char*) name);
    return function_search_id(functions, hashed);
}

/**
 * Search for a function with the specified function id.
 * @param functions function queue
 * @param id        the specified function's id
 * @return          NULL if no function of name found,
 *                  or the function structure if found
 */
function_t* function_search_id(queue_f* functions, uint64_t id) {
    // check if the function of requested id exists
    qnode_f *curr = functions->node;
    for (int i=0; i < functions->size; i++) {
        if (id == curr->function->id)
            break;
        curr = curr->next;
    }
    if (curr == NULL) return NULL;
    return curr->function;
}


/**
 * Free memory of given queue.
 * @param q  the queue
 */
 __attribute__((unused))
void free_function_queue(queue_f* q) {
    while (q->size > 0) function_dequeue(q);
    free(q);
}
