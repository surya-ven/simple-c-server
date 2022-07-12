/*
Author : Surya Venkatesh
Purpose: This file contains the functions that are used to handle 
         the server operations.
*/
#define _POSIX_C_SOURCE 200112L
#include "serverops.h"
#include "queue.h"
#include "connops.h"
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>

// Work queue for thread pool.
queue_t work_queue = { NULL, NULL };
pthread_mutex_t work_queue_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t work_queue_cond = PTHREAD_COND_INITIALIZER;

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
void init_server(int argc, char** argv) {
    int sockfd= 0, newsockfd = 0, s = 0;
	struct addrinfo* hints = NULL, * res = NULL;
	struct sockaddr_storage client_addr;
	socklen_t client_addr_size;
    struct thread_data thread_pool[THREAD_POOL_SIZE] = {0};

	if (argc < 4) {
		fprintf(stderr, "ERROR, not enough arguments provided\n");
		exit(EXIT_FAILURE);
	}

	// Convert argv[0] protocol number to integer
	int protocol = get_protocol(argv[1]);
	char* port = argv[2];
	char* root_path = argv[3];
    
    // Check if root path is empty or a space
    if (strlen(root_path) == 0 || strcmp(root_path, " ") == 0) {
        fprintf(stderr, "ERROR, root path is empty and thus not a valid \
            absolute path\n");
        exit(EXIT_FAILURE);
    }

	// Create address we're going to listen on (with given port number)
    hints = create_hints(protocol);
	
	// node (NULL means any interface), service (port), hints, res
	if ((s = getaddrinfo(NULL, port, hints, &res)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(s));
		exit(EXIT_FAILURE);
	}
    free(hints);

    sockfd = get_socket(res, protocol);

	// Listen on socket - means we're ready to accept connections,
	// incoming connection requests will be queued, man 3 listen
	if (listen(sockfd, BACKLOG_SIZE) < 0) {
		perror("listen");
		exit(EXIT_FAILURE);
	}
    // Print server is listening on port
    printf("Server is listening on port %s\n", port);

    // Create thread pool which work on the handle_work function
    int thread_count = create_thread_pool(thread_pool, THREAD_POOL_SIZE);
    if (thread_count <= 0) {
        fprintf(stderr, "ERROR: Could not create thread pool\n");
        exit(EXIT_FAILURE);
    }

    while (true) {
        // Accept a connection - blocks until a connection is ready to be accepted
        // Get back a new file descriptor to communicate on
        client_addr_size = sizeof client_addr;
        newsockfd = accept(sockfd, (struct sockaddr*)&client_addr, 
                        &client_addr_size);
        if (newsockfd < 0) {
            perror("accept");
            continue;
        }

        // Create work data for thread
        struct arg* w_arg = create_work_arg(newsockfd, root_path);
        if (w_arg == NULL) {
            close(newsockfd);
            fprintf(stderr, "ERROR: unable to create work arg\n");
            continue;
        }

        pthread_mutex_lock(&work_queue_mutex);
        // Add work data to work queue
        queue_enqueue(&work_queue, w_arg);
        // Signal threads to wake up and start working
        pthread_cond_signal(&work_queue_cond);
        pthread_mutex_unlock(&work_queue_mutex);
    }

	// Close socket
	close(sockfd);

    // Destroy mutex and cond
    pthread_mutex_destroy(&work_queue_mutex);
    pthread_cond_destroy(&work_queue_cond);
}

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
int get_protocol(char* protocol) {
    if (strcmp(protocol, IPv4_str) == 0) {
        return atoi(IPv4_str);
    } else if (strcmp(protocol, IPv6_str) == 0) {
        return atoi(IPv6_str);
    } else {
        fprintf(stderr, "Invalid protocol provided, defaulting to IPv4\n");
        return atoi(IPv4_str);
    }
}

/*
 * Function: is_valid_protocol
 * --------------------
 *  Checks if the protocol is valid.
 * 
 *  protocol: The protocol number.
 * 
 *  returns: True if valid, false otherwise.
 */
bool is_valid_protocol(int protocol) {
    if (protocol != atoi(IPv4_str) && protocol != atoi(IPv6_str)) {
        fprintf(stderr, "Invalid protocol provided, defaulting to IPv4\n");
        return false;
    }
    return true;
}

/*
 * Function: create_hints
 * --------------------
 *  Creates the hints struct for getaddrinfo.
 * 
 *  protocol: The protocol number.
 * 
 *  returns: The hints struct.
 */
struct addrinfo* create_hints(int protocol) {
    struct addrinfo* hints = malloc(sizeof(struct addrinfo));
    malloc_check(hints);
    memset(hints, 0, sizeof(struct addrinfo));

	if (protocol == 4) {
		hints->ai_family = AF_INET;
	} else if (protocol == 6) {
		hints->ai_family = AF_INET6;
	} else {
		fprintf(stderr, "ERROR, invalid protocol\n");
		exit(EXIT_FAILURE);
	}
	hints->ai_socktype = SOCK_STREAM; // TCP
	hints->ai_flags = AI_PASSIVE;     // for bind, listen, accept

    return hints;
}

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
int get_socket(struct addrinfo* res, int protocol) {
    if (res == NULL) {
        fprintf(stderr, "ERROR, no address info returned\n");
        exit(EXIT_FAILURE);
    }
    if (!is_valid_protocol(protocol)) {
        // Override protocol to IPv4 here
        protocol = atoi(IPv4_str);
    }

    int sockfd = -1;
    int re = 1;
    struct addrinfo* p = NULL;

    for (p = res; p != NULL; p = p->ai_next) {
		// Create socket if IPv4 or IPv6
		if (protocol == 4 && p->ai_family == AF_INET) {
			if ((sockfd = socket(p->ai_family, p->ai_socktype, 
				p->ai_protocol)) == -1) {
				perror("socket");
				continue;
			}
		} else if (protocol == 6 && p->ai_family == AF_INET6) {
			if ((sockfd = socket(p->ai_family, p->ai_socktype, 
				p->ai_protocol)) == -1) {
				perror("socket");
				continue;
			}
		} else {
			continue;
		}

		// Reuse port if possible
		if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &re, sizeof(int)) < 0) {
			perror("setsockopt");
			exit(EXIT_FAILURE);
		}

		// Bind address to the socket
		if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			close(sockfd);
			perror("bind");
			continue;
		}

		break;
	}
	freeaddrinfo(res);
	if (p == NULL)  {
        fprintf(stderr, "server: failed to bind\n");
        exit(EXIT_FAILURE);
    }
    
    return sockfd;
}

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
int create_thread_pool(struct thread_data* thread_pool, size_t len) {
    int t_status = 0;
    int thread_count = 0;
    pthread_attr_t attr;

    // Initialize thread attributes, set as detached
    if (pthread_attr_init(&attr) != 0) {
        fprintf(stderr, "ERROR: pthread_attr_init failed\n");
        exit(EXIT_FAILURE);
    }
    if (pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED) != 0) {
        fprintf(stderr, "ERROR: pthread_attr_setdetachstate failed\n");
        exit(EXIT_FAILURE);
    }
    
    // Create threads to work on handle_work function
    for (size_t i = 0; i < len; i++) {
        t_status = pthread_create(&(thread_pool[i].id), &attr, 
                    handle_work, NULL);
        thread_pool[i].status = t_status;

        // Check if thread was created successfully
        if (t_status != 0) {
            thread_pool[i].id = 0;
            continue;
        }

        thread_count++;
    }

    pthread_attr_destroy(&attr);

    return thread_count;
}

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
struct arg* create_work_arg(int clientfd, char* root_path) {
    struct arg* work_arg = malloc(sizeof(struct arg));
    malloc_check(work_arg);

    // Client socket file descriptor
    work_arg->clientfd = malloc(sizeof(int));
    malloc_check(work_arg->clientfd);
    *(work_arg->clientfd) = clientfd;

    // Root path
    work_arg->root_path = malloc(strlen(root_path) + 1);
    malloc_check(work_arg->root_path);
    strcpy(work_arg->root_path, root_path);

    return work_arg;
}

/*
 * Function: malloc_check
 * --------------------
 *  Checks if malloc was successful. If not, prints an error message and exits.
 * 
 *  ptr: The pointer to check.
 * 
 *  returns: Nothing.
 */
void malloc_check(void* ptr) {
    if (ptr == NULL) {
        fprintf(stderr, "ERROR, malloc failed\n");
        exit(EXIT_FAILURE);
    }
}

/*
 * Function: handle_work
 * --------------------
 *  Handles work for each thread.
 * 
 *  arg: unused, required by pthread_create.
 * 
 *  returns: The hints struct.
 */
void* handle_work(void* arg) {
    // Unused arg
    void* arg_unused __attribute__ ((unused)) = arg;

    void* queue_data = NULL;
    struct arg* work_arg = NULL;

    // Let each thread wait for work, and then process it when work is available
    while (true) {
        pthread_mutex_lock(&work_queue_mutex);
        if ((queue_data = queue_dequeue(&work_queue)) == NULL) {
            // Wait for work to be available
            pthread_cond_wait(&work_queue_cond, &work_queue_mutex);
        }
        pthread_mutex_unlock(&work_queue_mutex);

        if (queue_data != NULL) {
            work_arg = (struct arg*)queue_data;
            if (work_arg == NULL) {
                continue;
            }

            // Handle connection
            handle_client(work_arg);
            
            free_work_arg(work_arg);
        }
    }
    return NULL;
}

/*
 * Function: free_work_arg
 * --------------------
 *  Frees the work argument struct.
 * 
 *  work_arg: The work argument struct.
 * 
 *  returns: Nothing.
 */
void free_work_arg(struct arg* work_arg) {
    if (work_arg == NULL) return;
    free(work_arg->clientfd);
    free(work_arg->root_path);
    free(work_arg);
}