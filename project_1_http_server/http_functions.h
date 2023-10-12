// NXC Data Communications Network http_functions.h for HTTP server
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
#include "errno.h"

#define RED_PRTF(...) {printf("\033[0;31m"); printf(__VA_ARGS__); printf("\033[0m");}
#define GREEN_PRTF(...) {printf("\033[0;32m"); printf(__VA_ARGS__); printf("\033[0m");}
#define YELLOW_PRTF(...) {printf("\033[0;33m"); printf(__VA_ARGS__); printf("\033[0m");}
#define ERROR_PRTF(...) {fprintf(stderr, "\033[0;31m"); fprintf(stderr, "\t(%s:%d)", __FILE__, __LINE__); fprintf(stderr, __VA_ARGS__); fprintf (stderr, "\t(ERRNO %d: %s)\n", errno, strerror(errno)); fprintf(stderr, "\033[0m");}

#define MAX_HTTP_MSG_HEADER_SIZE 8*1024 // Maximum size of HTTP header.
#define DEFAULT_MAX_FIELD_NUM 8 

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


// PROJECT 1 - HTTP SERVER: IMPLEMENT THESE TWO FUNCTIONS

int server_engine (int server_port);
int server_routine (int server_port);

// You are NOT REQUIRED to implement and use parse_http_header().
// However, if you do, you will be able to use the http struct and its member functions,
// which will make things MUCH EASIER for you. We highly recommend you to do so.
http_t *parse_http_header (char *request);

/// HTTP MANIPULATION ///

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

// Format HTTP struct to HTTP, and stores it a on buffer. 
// Dynamically allocates memory for buffer.
// Returns number of bytes written if successful, -1 if not.
ssize_t write_http_to_buffer (http_t *http, void** buffer_ptr);

/// UTILITIES ///

// Prints a tuple of strings with a format.
void print_tuple_yellow (char *a, char *b, int format_len, int indent);

// Print HTTP struct to stdout.
void print_http_header (http_t *http);

// Print string with \r\n explicitly written.
void print_with_r_n (char *str);

// Encode data to base64.
// From https://stackoverflow.com/questions/342409/how-do-i-base64-encode-decode-in-c
char *base64_encode(char *data, size_t input_length) ;

// Alloc memory for string and copy it.
// Returns pointer to copied string if successful, NULL if not.
char *copy_string (char *string);

// Write size bytes from buffer to socket.
ssize_t write_bytes (int socket, void *buffer, size_t size);

// Read size bytes from socket to buffer.
ssize_t read_bytes (int socket, void *buffer, size_t size);

// Read a file and return its contents.
// Returns the size read if successful, -1 if not.
ssize_t read_file (void** output, char *file_path);

// Write a file with contents.
// Returns the size written if successful, -1 if not.
ssize_t write_file (char *file_path, void *data, size_t size);

// Append a file with contents.
// Returns the size written if successful, -1 if not.
ssize_t append_file (char *file_path, void *data, size_t size);

// Get file extension from file path. (ex. "index.html" -> "html", "image.jpg" -> "jpg")
// Returns pointer to file extension if successful, NULL if not.
// Does not include the dot, and does not allocate memory.
char *get_file_extension (char *file_path);


#endif // HTTP_FUNCTIONS_H