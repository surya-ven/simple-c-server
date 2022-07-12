#ifndef CONNOPS_H
#define CONNOPS_H

#include "connops.h"
#include "queue.h"
#include <stdlib.h>

#define BUFFER_LEN 2048
#define GET_METHOD "GET"
#define STATUS_OK "200"
#define STATUS_OK_M "OK"
#define STATUS_NF "404"
#define STATUS_NF_M "Not Found"
#define STATUS_FORBIDDEN "403"
#define STATUS_FORBIDDEN_M "Forbidden"
#define HTTP_VERSION "HTTP/1.0"
#define FILE_EXISTS 1
#define FILE_DOESNT_EXIST 0
#define STATUS_CODE_LEN 3
#define N_SPACES_REQ_LINE 2
#define N_SPACES_HEADER_LINE 1
#define MAX_CONTENT_TYPE_LEN 24
#define MAX_CONTENT_M_LEN 9
#define END_OF_REQUEST "\r\n\r\n"
#define END_OF_REQ_LINE "\r\n"
#define EXTRA_INCASE_NOSLASH 1
#define PATH_COMPONENT "/../"
#define ERROR -1
#define SUCCESS 0

/*
 * Function: file_stats
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
int file_stats(char* file_path_full, char* file_path, 
                char* content_type, size_t* file_size);

/*
 * Function: handle_client
 * --------------------
 *  Handles a client connection.
 * 
 *  arg: Pointer to the argument struct.
 * 
 *  returns: NULL.
 */
void* handle_client(void* arg);

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
int read_request(int clientfd, char* buffer);

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
                    char* file_path_full, size_t file_size);

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
    int clientfd, queue_t* free_queue);

/*
 * Function: path_component_exists
 * --------------------
 *  Checks if the path component exists.
 * 
 *  file_path: Path to the file.
 * 
 *  returns: true if the path component exists, false otherwise.
 */
bool path_component_exists(char* file_path);

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
                    char** file_path, char** protocol_version);

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
void malloc_check_close(void* ptr, int clientfd, queue_t* queue);

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
void* close_and_clean(int clientfd, queue_t* free_queue);


#endif