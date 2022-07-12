/*
Author : Surya Venkatesh
Purpose: This file contains the functions used for handling a 
         client connection.
*/
#include "connops.h"
#include "queue.h"
#include "serverops.h"
#include <netdb.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/sendfile.h>
#include <fcntl.h>

/*
 * Function: handle_client
 * --------------------
 *  Handles a client connection.
 * 
 *  arg: Pointer to the argument struct.
 * 
 *  returns: NULL.
 */
void* handle_client(void* arg) {
    if (arg == NULL) {
        return NULL;
    }
    // Make queue to free memory
    queue_t* free_queue = queue_create();
    struct arg* args = (struct arg*)arg;
    int clientfd = *(args->clientfd);
    char* root_path = args->root_path;
	char buffer[BUFFER_LEN + 1] = {0};
    
    // Read client request
    if (read_request(clientfd, buffer) != SUCCESS) {
        return close_and_clean(clientfd, free_queue);
    }

	// READ REQUEST LINE
	// Get only the first line of the request
    if (strstr(buffer, END_OF_REQ_LINE) == NULL) {
        fprintf(stderr, "ERROR, request line not found\n");
        return close_and_clean(clientfd, free_queue);
    }
    char* request_line = NULL, * method = NULL, 
        * file_path = NULL, * protocol_version = NULL;
    if (!parse_request(buffer, &request_line, &method, 
                        &file_path, &protocol_version)) {
        fprintf(stderr, "ERROR, malformed request provided\n");
        return close_and_clean(clientfd, free_queue);
    }

	// Write request log
	printf("%s %s %s\n", method, file_path, protocol_version);


	// Check if file exists in the root path
	char* file_path_full = malloc(sizeof(char) * (strlen(root_path) + 
                            strlen(file_path)) + EXTRA_INCASE_NOSLASH + 1);
    malloc_check_close(file_path_full, clientfd, free_queue);
    queue_enqueue(free_queue, file_path_full);

	strcpy(file_path_full, root_path);
    // Add trailing slash if not present at the start of file_path
    if (strlen(file_path) > 0 && file_path[0] != '/') {
        strcat(file_path_full, "/");
    }
	strcat(file_path_full, file_path);

    int file_status = 1;
    size_t file_size = 0;
    // Check if file path contains path component
    if (path_component_exists(file_path)) {
        // 404 - Invalid path provided
        file_status = 0;
    }

    // SEND RESPONSE
    char status_code[STATUS_CODE_LEN + 1] = {0};
    char content_type[MAX_CONTENT_TYPE_LEN + 1] = {0};
    char status_message[MAX_CONTENT_M_LEN + 1] = {0};

	if (file_status && (file_status = file_stats(file_path_full, file_path, 
                                content_type, &file_size))) {
		// File exists
		strcpy(status_code, STATUS_OK);
		strcpy(status_message, STATUS_OK_M);
	} else {
		// File doesn't exist
		strcpy(status_code, STATUS_NF);
		strcpy(status_message, STATUS_NF_M);
	}

    char* response = create_response_headers(file_status, status_code, 
                status_message, content_type, file_size, clientfd, free_queue);
    queue_enqueue(free_queue, response);

    // Send response headers and body
    if (send_response(clientfd, response, file_status, file_path_full, 
                    file_size) != SUCCESS) {
        return close_and_clean(clientfd, free_queue);
    }

    return close_and_clean(clientfd, free_queue);
}

/*
 * Function: send_response
 * --------------------
 *  Sends the response to the client.
 * 
 *  clientfd: Client file descriptor.
 *  response: Response to be sent.
 *  file_status: Status of the file.
 *  file_path_full: Full path of the file.
 *  file_size: Size of the file.
 * 
 *  returns: SUCCESS or ERROR.
 */
int send_response(int clientfd, char* response, int file_status, 
                    char* file_path_full, size_t file_size) {
    ssize_t n = 0;
    size_t total_sent = 0, bytes_left = strlen(response);
	while (total_sent < strlen(response)) {
        n = send(clientfd, response + total_sent, bytes_left, 0);

        if (n < 0) {
            perror("send");
            return ERROR;
        }
        // Client disconnected
        if (n == 0) {
            if (total_sent == strlen(response)) {
                break;
            }
            // Client disconnected before sending end of response
            return ERROR;
        }

        total_sent += n;
        bytes_left -= n;
    }

    int fd = -1;
	// Get file descriptor
	if (file_status) {
		// File exists
		fd = open(file_path_full, O_RDONLY);
		if (fd < 0) {
			perror("open");
            return ERROR;
		}

		// Send file with sendfile and offset
		off_t offset = 0;
        /*
        The benefits of sendfile:
        - Sendfile is more efficient than a combination of read and write 
        because it avoids the overhead of copying data to and from the user 
        space, as all copying is done within the kernel - this makes it 
        more performant than the alternative.
        - Additionally, sendfile is a more minimal approach to sending data 
        compared to a combination of read and write calls 
        given it is only one call.
        */
		while (sendfile(clientfd, fd, &offset, file_size) > 0);
		if (offset < 0) {
			perror("sendfile");
            return ERROR;
		}
		// Cast offset as it is positive
		if ((size_t)offset != file_size) {
			perror("sendfile");
            return ERROR;
		}

		// Close file descriptor
		close(fd);
	}
    return SUCCESS;
}

/*
 * Function: read_request
 * --------------------
 *  Reads the request from the client.
 * 
 *  clientfd: Client file descriptor.
 *  buffer: Buffer to store the request.
 * 
 *  returns: SUCCESS or ERROR.
 */
int read_request(int clientfd, char* buffer) {
    ssize_t n = 0;
    size_t total_recv = 0, bytes_left = BUFFER_LEN;
    // Read characters from the connection, then process
    // n is number of characters read
	while (total_recv < BUFFER_LEN) {
        n = recv(clientfd, buffer + total_recv, bytes_left, 0);

        // Check if there was an error reading from the connection
        if (n < 0) {
            perror("recv");
            return ERROR;
        }
        // Client disconnected
        if (n == 0) {
            if (strstr(buffer, END_OF_REQUEST) != NULL) {
                break;
            }
            // Client disconnected before sending end of request
            return ERROR;
        }

        total_recv += n;
        bytes_left -= n;

        // Check if buffer contains end of request
        if (strstr(buffer, END_OF_REQUEST) != NULL) {
            break;
        }

        // Check if buffer has been filled
        if (total_recv == BUFFER_LEN) {
            fprintf(stderr, "ERROR, buffer full\n");
            return ERROR;
        }
    }
    // Null-terminate string
    buffer[total_recv] = '\0';

    return SUCCESS; 
}

/*
 * Function: create_response_headers
 * --------------------
 *  Creates the response headers.
 * 
 *  file_status: Status of the file.
 *  status_code: Status code of the response.
 *  status_message: Status message of the response.
 *  content_type: Content type of the response.
 *  file_size: Size of the file.
 *  clientfd: Client file descriptor.
 *  free_queue: Free queue.
 * 
 *  returns: Pointer to the response headers.
 */
char* create_response_headers(int file_status, char* status_code, 
        char* status_message, char* content_type, size_t file_size, 
        int clientfd, queue_t* free_queue) {
    size_t response_len = 0, line_1_n = 0, line_2_n = 0, line_3_n = 0;
	
    // Get length of response headers
    if (file_status) {
        line_1_n = snprintf(NULL, 0, "%s %s %s\r\n", HTTP_VERSION, 
                        status_code, status_message); 
        line_2_n = snprintf(NULL, 0, "Content-Type: %s\r\n", content_type);
        line_3_n = snprintf(NULL, 0, "Content-Length: %zu\r\n\r\n", file_size);
        response_len = line_1_n + line_2_n + line_3_n;
    } else {
        line_1_n = snprintf(NULL, 0, "%s %s %s\r\n", HTTP_VERSION, 
                        status_code, status_message);
        line_3_n = snprintf(NULL, 0, "Content-Length: %zu\r\n\r\n", file_size);
        response_len = line_1_n + line_3_n;
    }

    char line_1[line_1_n + 1], line_2[line_2_n + 1], line_3[line_3_n + 1];
    memset(line_1, 0, line_1_n + 1);
    memset(line_2, 0, line_2_n + 1);
    memset(line_3, 0, line_3_n + 1);

    char* response = malloc(sizeof(char) * response_len + 1);
    malloc_check_close(response, clientfd, free_queue);

    // Create response header lines
    sprintf(line_1, "%s %s %s\r\n", HTTP_VERSION, status_code, 
                status_message);
    if (file_status) sprintf(line_2, "Content-Type: %s\r\n", content_type);
    sprintf(line_3, "Content-Length: %zu\r\n\r\n", file_size);

    // Create response
    strcpy(response, line_1);
    if (file_status) strcat(response, line_2);
    strcat(response, line_3);

    return response;
}

/*
 * Function: path_component_exists
 * --------------------
 *  Checks if the path component exists.
 * 
 *  file_path: Path to the file.
 * 
 *  returns: true if the path component exists, false otherwise.
 */
bool path_component_exists(char* file_path) {
    // Append "/" to start of file_path_copy if it doesn't exist
    // Add "/" to end for testing purposes
    char* file_path_copy = malloc(sizeof(char) * 
                            (strlen(file_path) + 
                            (2 * EXTRA_INCASE_NOSLASH) + 1));
    malloc_check(file_path_copy);

    strcpy(file_path_copy, "/");
    strcat(file_path_copy, file_path);
    strcat(file_path_copy, "/");

    // Check if file_path contains /../
    if (strstr(file_path_copy, PATH_COMPONENT)) {
        free(file_path_copy);
        return true;
    }
    free(file_path_copy);
    return false;
}

/*
 * Function: parse_requests
 * --------------------
 *  Parses the request.
 * 
 *  buffer: Buffer containing the request.
 *  request_line: Request line.
 *  method: Method of the request.
 *  file_path: Path to the file.
 *  protocol_version: Protocol version of the request.
 * 
 *  returns: true if the request was parsed successfully, false otherwise.
 */
bool parse_request(char* buffer, char** request_line, char** method, 
                    char** file_path, char** protocol_version) {
    // Get first line of request
    *request_line = strtok(buffer, END_OF_REQ_LINE);
    if (*request_line == NULL) {
        return false;
    }
	// Get the method
	*method = strtok(*request_line, " ");
    if (*method == NULL || strcmp(*method, GET_METHOD) != 0) {
        return false;
    }
	// Get the file path
	*file_path = strtok(NULL, " ");
    if (*file_path == NULL) {
        return false;
    }
	// Get the protocol
	*protocol_version = strtok(NULL, " ");
    if (*protocol_version == NULL || strcmp(*protocol_version, 
        HTTP_VERSION) != 0) {
        return false;
    }
    return true;
}

/*
 * Function: file_status
 * --------------------
 *  Checks if the file exists. If it does, it checks if it is a valid file, 
 *  then sets the content type and size of the file.
 * 
 *  file_path_full: Path to the file.
 *  content_type: Content type of the file.
 *  file_size: Size of the file.
 * 
 *  returns: 1 if the file exists, 0 otherwise.
 */
int file_stats(char* file_path_full, char* file_path, char* content_type, 
                size_t* file_size) {
    struct stat sb;
	if (stat(file_path_full, &sb) == 0) {
		// File exists
        // Get file size
        *file_size = sb.st_size;

        // Check if it's a regular file
        if (S_ISREG(sb.st_mode)) {
            if (strlen(file_path) == 0) return FILE_DOESNT_EXIST;
            // Get the last string starting with .
            char* file_path_noslash = file_path;
            char* extension = NULL;
            
            if (strstr(file_path, "/")) {
                if (strlen(strrchr(file_path, '/')) <= 1) return FILE_DOESNT_EXIST;
                file_path_noslash = strrchr(file_path, '/') + 1;
            }
            extension = strrchr(file_path_noslash, '.');

            // Check if it's an extensionless file 
            if (extension == NULL || strlen(extension) <= 1) {
                strcpy(content_type, "application/octet-stream");
            }
            // Check if it's a html file
            else if (strcmp(extension, ".html") == 0) {
                strcpy(content_type, "text/html");
            }
            // Check if it's a css file
            else if (strcmp(extension, ".css") == 0) {
                strcpy(content_type, "text/css");
            }
            // Check if it's a js file
            else if (strcmp(extension, ".js") == 0) {
                strcpy(content_type, "text/javascript");
            }
            // Check if it's a jpg file
            else if (strcmp(extension, ".jpg") == 0) {
                strcpy(content_type, "image/jpeg");
            } else {
                // Extension not recognized
                return FILE_DOESNT_EXIST;
            }
            // File exists and is a regular file
            return FILE_EXISTS;
        }
	}
    // Directory or file does not exist
	return FILE_DOESNT_EXIST;
}

/*
 * Function: malloc_check_close
 * --------------------
 *  Checks if malloc was successful. If it was not, it closes the socket and
 *  frees the queue, then exits.
 * 
 *  ptr: Pointer to the memory allocated.
 *  clientfd: Socket file descriptor.
 *  queue: free queue
 * 
 *  returns: Nothing.
 */
void malloc_check_close(void* ptr, int clientfd, queue_t* queue) {
    if (ptr == NULL) {
        fprintf(stderr, "ERROR, malloc failed\n");
        close_and_clean(clientfd, queue);
        exit(EXIT_FAILURE);
    }
}

/*
 * Function: close_and_clean
 * --------------------
 *  Closes the socket and frees the queue.
 * 
 *  clientfd: Socket file descriptor.
 *  free_queue: Free queue.
 * 
 *  returns: NULL.
 */
void* close_and_clean(int clientfd, queue_t* free_queue) {
    close(clientfd);
    queue_clean(free_queue, free);
    return NULL;
}