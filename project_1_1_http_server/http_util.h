// NXC Data Communications http_util.h
// Written by Jongseok Park (cakeng@snu.ac.kr)
// 2023. 9. 11

#ifndef HTTP_UTIL_H
#define HTTP_UTIL_H

#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "unistd.h"

#define MAX_HTTP_MSG_HEADER_SIZE 4096 // Maximum size of HTTP GET message

// All pointer locations should be dynamically allocated.
typedef struct http_field_t
{
    char *field;
    char *val;
} http_field_t;

// All pointer locations should be dynamically allocated.
typedef struct http_t
{
    char *method;
    char *path;
    char *version;
    char *status;
    size_t body_size;
    void *body_data;
    size_t field_count;
    size_t max_field_count;
    http_field_t *fields;
} http_t;

// Write size bytes from buffer to sock.
ssize_t write_bytes (int sock, char *buffer, size_t size);

// Read size bytes from sock to buffer.
ssize_t read_bytes (int sock, char *buffer, size_t size);

// Parses the header of a HTTP string.
// Returns parsed http_t struct if successful, NULL if not.
http_t *parse_http_header (char *request);

// Create an empty HTTP response message struct with version and status.
// Returns NULL if not successful.
http_t *init_http_response (char *version, char *status);

// Add a field to HTTP struct.
// Returns 0 if successful, -1 if not.
int add_field_to_http (http_t **http_ptr, char *field, char *val);

// Add a body to HTTP struct. Includes addition of Content-Length field.
// Skips if body_size is 0 or body_data is NULL.
// Returns 0 if successful, -1 if not.
int add_body_to_http (http_t **http_ptr, size_t body_size, void *body_data);

// Find value of field in HTTP request header.
// Returns pointer to value if successful, NULL if not.
char *find_http_request_header_field (http_t *http, char *field);

// Format http_t struct to buffer. Dynamically allocates memory for buffer.
// Returns number of bytes written if successful, -1 if not.
ssize_t write_http_to_buffer (http_t *http, void** buffer_ptr);

// Deep-copy http_t struct.
// Returns pointer to copied http_t struct if successful, NULL if not.
http_t *copy_http (http_t *http);

// Free http_t struct.
void free_http (http_t *http);

// Print http_t struct to stdout.
void print_http (http_t *http);

#endif // HTTP_UTIL_H