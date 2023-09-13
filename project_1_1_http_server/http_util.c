// NXC Data Communications http_util.c
// Written by Jongseok Park (cakeng@snu.ac.kr)
// 2023. 9. 11

#include "http_util.h"

// Write size bytes from buffer to sock.
ssize_t write_bytes (int sock, char *buffer, size_t size)
{
    ssize_t bytes_sent = 0;
    ssize_t bytes_remaining = size;
    while (bytes_remaining > 0)
    {
        bytes_sent = write(sock, buffer, bytes_remaining);
        if (bytes_sent == -1)
        {
            printf("write() error\n");
            return -1;
        }
        bytes_remaining -= bytes_sent;
        buffer += bytes_sent;
    }
    return size;
}

// Read size bytes from sock to buffer.
ssize_t read_bytes (int sock, char *buffer, size_t size)
{
    ssize_t bytes_received = 0;
    ssize_t bytes_remaining = size;
    while (bytes_remaining > 0)
    {
        bytes_received = read(sock, buffer, bytes_remaining);
        if (bytes_received == -1)
        {
            printf("read() error\n");
            return -1;
        }
        bytes_remaining -= bytes_received;
        buffer += bytes_received;
    }
    return size;
}

// Parses the header of a HTTP string.
// Returns parsed http_t struct if successful, NULL if not.
http_t *parse_http_header (char *request)
{
    if (request == NULL)
    {
        printf("parse_http_header() NULL parameter error\n");
        return NULL;
    }

    http_t *http = (http_t *) malloc(sizeof(http_t));
    if (http == NULL)
    {
        printf("parse_http_header() malloc() error\n");
        return NULL;
    }

    http->method = NULL;
    http->path = NULL;
    http->version = NULL;
    http->status = NULL;
    http->body_size = 0;
    http->body_data = NULL;
    http->field_count = 0;
    http->max_field_count = 16;
    http->fields = (http_field_t *) malloc(http->max_field_count * sizeof(http_field_t));


    return http;
}

// Create an empty HTTP response message with version and status.
// Returns NULL if not successful.
http_t *init_http_response (char *version, char *status)
{
    if (version == NULL || status == NULL)
    {
        printf("init_http_response() NULL parameter error\n");
        return NULL;
    }

    http_t *response = (http_t *) malloc(sizeof(http_t));
    if (response == NULL)
    {
        printf("init_http_response() malloc() error\n");
        return NULL;
    }

    response->method = NULL;
    response->path = NULL;

    response->version = (char *) malloc(strlen(version) + 1);
    if (response->version == NULL)
    {
        printf("init_http_response() version malloc() error\n");
        return NULL;
    }
    strcpy(response->version, version);

    response->status = (char *) malloc(strlen(status) + 1);
    if (response->status == NULL)
    {
        printf("init_http_response() status malloc() error\n");
        return NULL;
    }
    strcpy(response->status, status);

    response->body_size = 0;
    response->body_data = NULL;

    response->field_count = 0;
    response->max_field_count = 16;
    response->fields = (http_field_t *) malloc(response->max_field_count * sizeof(http_field_t));

    return response;
}

// Add a field to HTTP struct.
// Returns 0 if successful, -1 if not.
int add_field_to_http (http_t **http_ptr, char *field, char *val)
{
    if (http_ptr == NULL || *http_ptr == NULL || field == NULL || val == NULL)
    {
        printf("add_field_to_http_response_header() NULL parameter error\n");
        return -1;
    }

    http_t *response_ptr = *http_ptr;
    if (find_http_request_header_field(response_ptr, field) != NULL)
    {
        printf("add_field_to_http_response_header() field already exists error\n");
        return -1;
    }

    response_ptr->field_count++;
    if (response_ptr->field_count > response_ptr->max_field_count)
    {
        response_ptr->max_field_count *= 2;
        response_ptr->fields = (http_field_t *) realloc(response_ptr->fields, response_ptr->max_field_count * sizeof(http_field_t));
        if (response_ptr->fields == NULL)
        {
            printf("add_field_to_http_response_header() fields realloc() error\n");
            return -1;
        }
    }

    response_ptr->fields[response_ptr->field_count - 1].field = (char *) malloc(strlen(field) + 1);
    if (response_ptr->fields[response_ptr->field_count - 1].field == NULL)
    {
        printf("add_field_to_http_response_header() field malloc() error\n");
        return -1;
    }
    strcpy(response_ptr->fields[response_ptr->field_count - 1].field, field);

    response_ptr->fields[response_ptr->field_count - 1].val = (char *) malloc(strlen(val) + 1);
    if (response_ptr->fields[response_ptr->field_count - 1].val == NULL)
    {
        printf("add_field_to_http_response_header() val malloc() error\n");
        return -1;
    }
    strcpy(response_ptr->fields[response_ptr->field_count - 1].val, val);

    return 0;
}

// Add a body to HTTP struct. Includes addition of Content-Length field.
// Skips if body_size is 0 or body_data is NULL.
// Returns 0 if successful, -1 if not.
int add_body_to_http (http_t **http_ptr, size_t body_size, void *body_data)
{
    if (http_ptr == NULL || *http_ptr == NULL)
    {
        printf("add_body_to_http_response() NULL parameter error\n");
        return -1;
    }
    if (body_size == 0 || body_data == NULL)
        return 0;

    http_t *response_ptr = *http_ptr;
    if (response_ptr->body_data != NULL || response_ptr->body_size != 0)
    {
        printf("add_body_to_http_response() body_data already exists error\n");
        return -1;
    }

    char content_length[32] = {0};
    sprintf(content_length, "%lu", body_size);
    if (add_field_to_http (http_ptr, "Content-Length", content_length) == -1)
    {
        printf("add_body_to_http_response() add_field_to_http_response_header() error\n");
        return -1;
    }

    response_ptr->body_size = body_size;
    response_ptr->body_data = (void *) malloc(body_size);
    if (response_ptr->body_data == NULL)
    {
        printf("add_body_to_http_response() body_data malloc() error\n");
        return -1;
    }
    memcpy(response_ptr->body_data, body_data, body_size);

    return 0;
}

// Find value of field in HTTP request header.
// Returns pointer to value if successful, NULL if not.
char *find_http_request_header_field (http_t *http, char *field)
{
    if (http == NULL || field == NULL)
    {
        printf("find_http_request_header_field() NULL parameter error\n");
        return NULL;
    }
    for (int i = 0; i < http->field_count; i++)
    {
        if (strcmp(http->fields[i].field, field) == 0)
            return http->fields[i].val;
    }
    return NULL;
}

// Format http_t struct to buffer. Dynamically allocates memory for buffer.
// Returns number of bytes written if successful, -1 if not.
ssize_t write_http_to_buffer (http_t *http, void** buffer_ptr)
{
    size_t buffer_size = 0;
    if (http == NULL || buffer_ptr == NULL)
    {
        printf("write_http_to_buffer() NULL parameter error\n");
        return -1;
    }
    if (http->version == NULL)
    {
        printf("write_http_to_buffer() NULL version error\n");
        return -1;
    }
    buffer_size += strlen(http->version) + 1;
    buffer_size += http->method? strlen(http->method) + 1 : 0;
    buffer_size += http->path? strlen(http->path) + 1 : 0;
    buffer_size += http->status? strlen(http->status) + 1 : 0;
    buffer_size += 1; // \r\n

    for (int i = 0; i < http->field_count; i++)
        buffer_size += strlen(http->fields[i].field) + 2 + strlen(http->fields[i].val) + 2; // field: val\r\n
    buffer_size += 2; // \r\n

    buffer_size += http->body_size;

    if (*buffer_ptr != NULL)
        free(*buffer_ptr);
    *buffer_ptr = (void *) malloc(buffer_size);
    printf ("buffer_size: %lu\n", buffer_size);
    if (*buffer_ptr == NULL)
    {
        printf("write_http_to_buffer() buffer malloc() error\n");
        return -1;
    }
    char *buffer = (char *) *buffer_ptr;
    buffer[0] = '\0';
    
    strcat (buffer, http->method? http->method : "");
    strcat (buffer, http->method? " " : "");
    strcat (buffer, http->path? http->path : "");
    strcat (buffer, http->path? " " : "");
    strcat (buffer, http->version);
    strcat (buffer, " ");
    strcat (buffer, http->status? http->status : "");
    strcat (buffer, "\r\n");
    for (int i = 0; i < http->field_count; i++)
    {
        strcat (buffer, http->fields[i].field);
        strcat (buffer, ": ");
        strcat (buffer, http->fields[i].val);
        strcat (buffer, "\r\n");
    }
    strcat (buffer, "\r\n");
    buffer += strlen(buffer);
    if ((char *) *buffer_ptr + buffer_size != http->body_size + buffer)
    {
        printf("write_http_to_buffer() buffer size error\n");
        return -1;
    }       
    if (http->body_data)
    {
        memcpy(buffer, http->body_data, http->body_size);
        buffer += http->body_size;
    }
    return buffer_size;
}

// Copy http_t struct.
// Returns pointer to copied http_t struct if successful, NULL if not.
http_t *copy_http (http_t *http)
{
    if (http == NULL)
    {
        printf("copy_http() NULL parameter error\n");
        return NULL;
    }

    http_t *copy = (http_t *) malloc(sizeof(http_t));
    if (copy == NULL)
    {
        printf("copy_http() malloc() error\n");
        return NULL;
    }

    copy->method = NULL;
    copy->path = NULL;
    copy->version = NULL;
    copy->status = NULL;
    copy->body_size = http->body_size;
    copy->body_data = NULL;
    copy->field_count = http->field_count;
    copy->max_field_count = http->max_field_count;
    copy->fields = (http_field_t *) malloc(copy->max_field_count * sizeof(http_field_t));
    if (copy->fields == NULL)
    {
        printf("copy_http() fields malloc() error\n");
        return NULL;
    }

    if (http->method)
    {
        copy->method = (char *) malloc(strlen(http->method) + 1);
        if (copy->method == NULL)
        {
            printf("copy_http() method malloc() error\n");
            return NULL;
        }
        strcpy(copy->method, http->method);
    }

    if (http->path)
    {
        copy->path = (char *) malloc(strlen(http->path) + 1);
        if (copy->path == NULL)
        {
            printf("copy_http() path malloc() error\n");
            return NULL;
        }
        strcpy(copy->path, http->path);
    }

    if (http->version)
    {
        copy->version = (char *) malloc(strlen(http->version) + 1);
        if (copy->version == NULL)
        {
            printf("copy_http() version malloc() error\n");
            return NULL;
        }
        strcpy(copy->version, http->version);
    }

    if (http->status)
    {
        copy->status = (char *) malloc(strlen(http->status) + 1);
        if (copy->status == NULL)
        {
            printf("copy_http() status malloc() error\n");
            return NULL;
        }
        strcpy(copy->status, http->status);
    }

    if (http->body_data)
    {
        copy->body_data = (void *) malloc(http->body_size);
        if (copy->body_data == NULL)
        {
            printf("copy_http() body_data malloc() error\n");
            return NULL;
        }
        memcpy(copy->body_data, http->body_data, http->body_size);
    }

    for (int i = 0; i < http->field_count; i++)
    {
        if (http->fields[i].field == NULL || http->fields[i].val == NULL)
        {
            printf("copy_http() NULL field or val error at idx %d\n", i);
            return NULL;
        }
        copy->fields[i].field = (char *) malloc(strlen(http->fields[i].field) + 1);
        if (copy->fields[i].field == NULL)
        {
            printf("copy_http() field malloc() error\n");
            return NULL;
        }
        strcpy(copy->fields[i].field, http->fields[i].field);
        copy->fields[i].val = (char *) malloc(strlen(http->fields[i].val) + 1);
        if (copy->fields[i].val == NULL)
        {
            printf("copy_http() val malloc() error\n");
            return NULL;
        }
        strcpy(copy->fields[i].val, http->fields[i].val);
    }

    return copy;
}


// Free http_t struct.
void free_http (http_t *http)
{
    if (http == NULL)
        return;
    if (http->method)
        free(http->method);
    if (http->path)
        free(http->path);
    if (http->version)
        free(http->version);
    if (http->status)
        free(http->status);
    if (http->body_data)
        free(http->body_data);
    for (int i = 0; i < http->field_count; i++)
    {
        if (http->fields[i].field)
            free(http->fields[i].field);
        if (http->fields[i].val)
            free(http->fields[i].val);
    }
    free(http->fields);
    free(http);
}

// Print http_t struct to stdout.
void print_http (http_t *http)
{
    if (http == NULL)
    {
        printf("print_http() NULL parameter error\n");
        return;
    }
    printf("//// PRINTING HTTP_T STRUCT ////\n");
    printf("\tmethod: %s\n", http->method? http->method : "NULL");
    printf("\tpath: %s\n", http->path? http->path : "NULL");
    printf("\tversion: %s\n", http->version? http->version : "NULL");
    printf("\tstatus: %s\n", http->status? http->status : "NULL");
    printf("\tbody_size: %lu\n", http->body_size);
    printf("\tfield_count: %lu\n", http->field_count);
    printf("\tfields: %p\n", http->fields);
    for (int i = 0; i < http->field_count; i++)
        printf("\t\t%s: %s\n", http->fields[i].field? http->fields[i].field : "NULL",
                http->fields[i].val? http->fields[i].val : "NULL");
    printf("//// END OF HTTP_T STRUCT ////\n");
}
