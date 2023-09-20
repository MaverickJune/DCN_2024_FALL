// NXC Data Communications http_functions.h for HTTP server
// Written by Jongseok Park (cakeng@snu.ac.kr)
// 2023. 9. 11


///// DO NOT MODIFY THIS FILE!! ////

#ifndef HTTP_FUNCTIONS_H
#define HTTP_FUNCTIONS_H

#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "unistd.h"
#include "signal.h"
#include "stdint.h"

#define MAX_HTTP_MSG_HEADER_SIZE 4096 // Maximum size of HTTP GET message
#define DEFAULT_MAX_FIELD_NUM 8 

int server_engine (int server_port);
int server_routine (int server_port);


/// HTTP MANIPULATION ///
// Refer to http_util.c for details.

// Struct for HTTP field.
// All pointer locations should be dynamically allocated.
typedef struct http_field_t
{
    char *field;
    char *val;
} http_field_t;

// Struct for HTTP message.
// All pointer locations should be dynamically allocated.
typedef struct http_t
{
    char *method;
    char *path;
    char *version;
    char *status;

    size_t body_size;
    void *body_data;

    int field_count;
    int max_field_count;
    http_field_t *fields;
} http_t;

// Create an empty HTTP struct
// Returns NULL if not successful.
http_t *init_http ();

// Create an empty HTTP struct with status/request line fields.
// Skips arguments if NULL.
// Returns NULL if not successful.
http_t *init_http_with_arg (char *method, char *path, char *version, char *status);

// Deep-copy HTTP struct.
// Returns pointer to copied HTTP struct if successful, NULL if not.
http_t *copy_http (http_t *http);

// Free HTTP struct.
void free_http (http_t *http);

// Find value of a header field in a HTTP struct.
// Returns pointer to value if successful, NULL if not.
char *find_http_field_val (http_t *http, char *field);

// Add a header field to HTTP struct.
// Returns 0 if successful, -1 if not.
int add_field_to_http (http_t *http, char *field, char *val);

// Remove a header field from HTTP struct.
// Returns 0 if successful, -1 if not.
int remove_field_from_http (http_t *http, char *field);

// Add a body to HTTP struct. Includes addition of Content-Length header field.
// Skips if body_size is 0 or body_data is NULL.
// Returns 0 if successful, -1 if not.
int add_body_to_http (http_t *http, size_t body_size, void *body_data);

// Remove a body from HTTP struct.
// Returns 0 if successful, -1 if not.
int remove_body_from_http (http_t *http);

// Parses a HTTP header string.
// Returns parsed HTTP struct if successful, NULL if not..
http_t *parse_http_header (char *request);

// Format HTTP struct to HTTP, and stores it a on buffer. 
// Dynamically allocates memory for buffer.
// Returns number of bytes written if successful, -1 if not.
ssize_t write_http_to_buffer (http_t *http, void** buffer_ptr);


/// UTILITIES ///
// Refer to http_util.c for details.

// Print HTTP struct to stdout.
void print_http_header (http_t *http);

// Encode data to base64.
// From https://stackoverflow.com/questions/342409/how-do-i-base64-encode-decode-in-c
char *base64_encode(char *data, size_t input_length) ;

// Alloc memory for string and copy it.
// Returns pointer to copied string if successful, NULL if not.
char *copy_string (char *string);

// Write size bytes from buffer to socket.
ssize_t write_bytes (int socket, char *buffer, size_t size);

// Read size bytes from socket to buffer.
ssize_t read_bytes (int socket, char *buffer, size_t size);

// Read a file and return its contents.
// Returns the size of the file if successful, -1 if not.
ssize_t read_file (void** output, char *file_path);

// Get file extension from file path. (ex. "index.html" -> "html", "image.jpg" -> "jpg")
// Returns pointer to file extension if successful, NULL if not.
// Does not include the dot, and does not allocate memory.
char *get_file_extension (char *file_path);


#endif // HTTP_FUNCTIONS_H