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
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include "function_queue.h"


/**
 * DJB2 hash to hash strings.
 * @param str specified string
 * @return    hashed string value
 */
unsigned long hash(unsigned char* str) {
    unsigned long hash_val = 5381;
    int c;
    while ((c = *str++))
        hash_val = ((hash_val << 5) + hash_val) + c;
    return hash_val;
}


/**
 * Initialize a function.
 * @param f_name    the function's name
 * @param f_handler the function's handler
 * @return          the function
 */
function_t* function_init(char* f_name, rpc_handler f_handler) {
    unsigned long name_len = strlen(f_name);
    if (name_len < MIN_NAME_LEN) {
        fprintf(stderr, "function_init - name length = %lu, "
                        "below minimal threshold %d\n",
                name_len, MIN_NAME_LEN);
        return NULL;
    }
    for (int i=0; i < strlen(f_name); i++) {
        char character = f_name[i];
        int ascii = (int) character;
        if (ascii < 32 || ascii > 126) {
            fprintf(stderr, "function_init - ascii character %c (ascii %d) "
                            "is not within range 32 and 126\n",
                    character, ascii);
            return NULL;
        }
    }
    function_t* f = (function_t*) malloc(sizeof(function_t));
    assert(f);
    unsigned long id = hash((unsigned char*) f_name);
    f->id = id;
    f->f_name = f_name;
    f->f_handler = f_handler;
    assert(f->f_name && f->f_handler);
    return f;
}


/**
 * Initialize a queue's node.
 * @return  the initialized queue node
 */
qnode_f* qnode_init() {
    qnode_f* qn = (qnode_f*) malloc(sizeof(qnode_f));
    assert(qn);
    return qn;
}

/**
 * Initialize a function queue.
 * @return  the initialized queue
 */
queue_f* queue_init() {
    queue_f* q = (queue_f*) malloc(sizeof(queue_f));
    q->node = qnode_init();
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
int enqueue(queue_f* q, function_t* f) {
    assert(q);
    assert(q->last);
    if (! f) return -1;

    // ensure uniqueness in registration
    qnode_f* curr = q->node;
    for (int i=0; i < q->size; i++) {
        function_t* f_curr = curr->function;
        if (f_curr->id == f->id) {
            // just for debugging
            assert(strcmp(f_curr->f_name, f->f_name) == 0);
            printf("server: function_queue: function %s already exists", f->f_name);
            return 1;
        }
        assert(strcmp(f_curr->f_name, f->f_name) != 0);
        curr = curr->next;
    }
    q->last->function = f;
    q->last->next = qnode_init();
    q->last = q->last->next;
    (q->size)++;
    return 0;
}

/**
 * Dequeue a process from given queue.
 * @param q the given queue
 * @return  the dequeued function
 */
function_t* dequeue(queue_f* q) {
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
 * Free memory of given queue.
 * @param q  the queue
 */
void free_queue(queue_f* q) {
    while (q->size > 0) dequeue(q);
    free(q);
}
