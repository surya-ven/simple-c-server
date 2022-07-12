#ifndef SERVEROPS_H
#define SERVEROPS_H

#include <stdlib.h>
#include <pthread.h>
#include <stdbool.h>
#include "queue.h"

#define IPv4_str "4"
#define IPv6_str "6"
#define RANDOM_PORT "0"
#define BACKLOG_SIZE 10
#define THREAD_POOL_SIZE 10
#define VALID_THREAD 0

struct arg {
    int* clientfd;
    char* root_path;
};

struct thread_data {
    int status;
    pthread_t id;
};

/*
 * Function: init_server
 * --------------------
 *  Initializes the server.
 * 
 *  argc: The number of arguments passed to the program.
 *  argv: The array of arguments passed to the program.
 * 
 *  returns: Nothing.
 */
void init_server(int argc, char** argv);

/*
 * Function: get_protocol
 * --------------------
 *  Converts the protocol string to an integer, and also
 *  validates the protocol. Defaults to IPv4 if invalid.
 * 
 *  protocol: The protocol string.
 * 
 *  returns: The protocol number.
 */
int get_protocol(char* protocol);

/*
 * Function: is_valid_protocol
 * --------------------
 *  Checks if the protocol is valid.
 * 
 *  protocol: The protocol number.
 * 
 *  returns: True if valid, false otherwise.
 */
bool valid_protocol(int protocol);

/*
 * Function: create_hints
 * --------------------
 *  Creates the hints struct for getaddrinfo.
 * 
 *  protocol: The protocol number.
 * 
 *  returns: The hints struct.
 */
struct addrinfo* create_hints(int protocol);

/*
 * Function: get_socket
 * --------------------
 *  Creates a socket and binds it to the given address.
 * 
 *  res: The address to bind to.
 *  protocol: The protocol number.
 * 
 *  returns: The socket file descriptor.
 */
int get_socket(struct addrinfo* res, int protocol);

/*
 * Function: create_thread_pool
 * --------------------
 *  Creates a thread pool of the given size.
 * 
 *  thread_pool: The thread pool struct.
 *  len: The number of threads to create.
 * 
 *  returns: The number of threads created.
 */
int create_thread_pool(struct thread_data thread_pool[], size_t len);

/*
 * Function: malloc_check
 * --------------------
 *  Checks if malloc was successful. If not, prints an error message and exits.
 * 
 *  ptr: The pointer to check.
 * 
 *  returns: Nothing.
 */
void malloc_check(void* ptr);

/*
 * Function: create_work_arg
 * --------------------
 *  Creates a work argument struct.
 * 
 *  clienfd: The client socket file descriptor.
 *  root_path: The root path of the server.
 * 
 *  returns: The argument struct.
 */
struct arg* create_work_arg(int clientfd, char* root_path);

/*
 * Function: free_work_arg
 * --------------------
 *  Frees the work argument struct.
 * 
 *  work_arg: The work argument struct.
 * 
 *  returns: Nothing.
 */
void free_work_arg(struct arg* work_arg);

/*
 * Function: handle_work
 * --------------------
 *  Handles work for each thread.
 * 
 *  arg: unused, required by pthread_create.
 * 
 *  returns: The hints struct.
 */
void* handle_work(void* arg);


#endif