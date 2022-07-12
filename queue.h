#ifndef QUEUE_H
#define QUEUE_H

#include <stdlib.h>
#include <stdbool.h>

typedef void (* free_queue_t)(void*);
typedef void* (* get_next_t)(void*);

typedef struct queue_node queue_node_t;

struct queue_node {
    void* data;
    queue_node_t* next;
};

typedef struct queue {
    queue_node_t* head;
    queue_node_t* tail;
} queue_t;

/*
 * Function: queue_create
 * --------------------
 *  Creates a new queue.
 * 
 *  No parameters.
 * 
 *  returns: Pointer to the new queue.
 */
queue_t* queue_create(void);

/*
 * Function: queue_create_from_array
 * --------------------
 *  Creates a new queue from given array.
 * 
 *  node_array: Array to create queue from.
 *  n_nodes: Number of nodes in array.
 * 
 *  returns: Pointer to the new queue.
 */
queue_t* queue_create_from_array(void** node_array, size_t n_nodes);

/*
 * Function: queue_enqueue
 * --------------------
 *  enqueuees a new node to the queue.
 * 
 *  queue: Pointer to the queue.
 *  data: Data to insert.
 * 
 *  returns: Nothing.
 */
void queue_enqueue(queue_t* queue, void* data);

/*
 * Function: queue_peek
 * --------------------
 *  Peeks the top node from the queue.
 * 
 *  queue: Pointer to the queue.
 * 
 *  returns: Pointer to the data of the top node.
 */
void* queue_peek(queue_t* queue);

/*
 * Function: queue_dequeue
 * --------------------
 *  dequeues the top node from the queue.
 * 
 *  queue: Pointer to the queue.
 * 
 *  returns: Pointer to the data of the dequeueped node.
 */
void* queue_dequeue(queue_t* queue);

/*
 * Function: queue_dequeue_free
 * --------------------
 *  dequeues the top node from the queue and frees the data if appropriate 
 *  function is given.
 * 
 *  queue: Pointer to the queue.
 *  free_data: Function pointer to free the data.
 * 
 *  returns: Nothing.
 */
void queue_dequeue_free(queue_t* queue, free_queue_t free_data);

/*
 * Function: queue_is_empty
 * --------------------
 *  Checks if the queue is empty.
 * 
 *  queue: Pointer to the queue.
 * 
 *  returns: True if queue is empty, false otherwise.
 */
bool queue_is_empty(queue_t* queue);

/*
 * Function: queue_clean
 * --------------------
 *  Cleans the queue.
 * 
 *  queue: Pointer to the queue.
 *  free_data: Function pointer to free the data.
 * 
 *  returns: Nothing.
 */
void queue_clean(queue_t* queue, free_queue_t free_data);

#endif