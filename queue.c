/*
Author : Surya Venkatesh
Purpose: This file is a custom queue library.
*/

#include "queue.h"
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>

/*
 * Function: queue_create
 * --------------------
 *  Creates a new queue.
 * 
 *  No parameters.
 * 
 *  returns: Pointer to the new queue.
 */
queue_t* queue_create(void) {
    queue_t* queue = malloc(sizeof(queue_t));
    if (queue == NULL) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }
    queue->head = NULL;
    queue->tail = NULL;
    return queue;
}

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
queue_t* queue_create_from_array(void** node_array, size_t n_nodes) {
    queue_t* queue = malloc(sizeof(queue_t));
    if (queue == NULL) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }
    queue->head = NULL;
    queue->tail = NULL;

    for (size_t i = 0; i < n_nodes; i++) {
        queue_enqueue(queue, node_array[i]);
    }

    return queue;
}

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
void queue_enqueue(queue_t* queue, void* data) {
    if (queue == NULL) {
        return;
    }

    queue_node_t* node = malloc(sizeof(queue_node_t));
    if (node == NULL) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }

    node->data = data;
    node->next = NULL;

    if (queue->head == NULL) {
        queue->head = node;
    } else {
        queue->tail->next = node;
    }
    queue->tail = node;
}

/*
 * Function: queue_dequeue
 * --------------------
 *  dequeues the top node from the queue.
 * 
 *  queue: Pointer to the queue.
 * 
 *  returns: Pointer to the data of the dequeueped node.
 */
void* queue_dequeue(queue_t* queue) {
    if (queue == NULL) {
        return NULL;
    } else if (queue->head == NULL) {
        return NULL;
    }

    queue_node_t* node = queue->head;
    queue->head = node->next;

    void* data = node->data;
    free(node);

    if (queue->head == NULL) {
        queue->tail = NULL;
    }

    return data;
}

/*
 * Function: queue_peek
 * --------------------
 *  Peeks the top node from the queue.
 * 
 *  queue: Pointer to the queue.
 * 
 *  returns: Pointer to the data of the top node.
 */
void* queue_peek(queue_t* queue) {
    if (queue == NULL) {
        return NULL;
    } else if (queue->head == NULL) {
        return NULL;
    }

    return queue->head->data;
}

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
void queue_dequeue_free(queue_t* queue, free_queue_t free_data) {
    if (queue == NULL) {
        return;
    } else if (queue->head == NULL) {
        return;
    }

    queue_node_t* node = queue->head;
    queue->head = node->next;

    if (free_data != NULL) {
        free_data(node->data);
    }
    free(node);

    if (queue->head == NULL) {
        queue->tail = NULL;
    }
}

/*
 * Function: queue_is_empty
 * --------------------
 *  Checks if the queue is empty.
 * 
 *  queue: Pointer to the queue.
 * 
 *  returns: True if queue is empty, false otherwise.
 */
bool queue_is_empty(queue_t* queue) {
    if (queue == NULL) {
        return true;
    } else if (queue->head == NULL) {
        return true;
    }
    return false;
}

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
void queue_clean(queue_t* queue, free_queue_t free_data) {
    if (queue == NULL) {
        return;
    }

    while (!queue_is_empty(queue)) {
        queue_dequeue_free(queue, free_data);
    }

    free(queue);
}