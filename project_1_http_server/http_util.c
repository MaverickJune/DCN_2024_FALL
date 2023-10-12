// NXC Data Communications Network http_util.c for HTTP server
// Written by Jongseok Park (cakeng@snu.ac.kr)
// 2023. 9. 11


///// DO NOT MODIFY THIS FILE!! ////

#include "http_functions.h"

http_t *init_http ()
{
    http_t *http = (http_t *) calloc(1, sizeof(http_t));
    if (http == NULL)
    {
        ERROR_PRTF ("ERROR init_http_response(): calloc() (ERRNO %d:%s)\n", errno, strerror(errno));
        return NULL;
    }
    http->method = NULL;
    http->path = NULL;
    http->version = NULL;
    http->status = NULL;
    http->body_size = 0;
    http->body_data = NULL;
    http->field_count = 0;
    http->max_field_count = DEFAULT_MAX_FIELD_NUM;
    http->fields = (http_field_t *) calloc(http->max_field_count, sizeof(http_field_t));
    if (http->fields == NULL)
    {
        ERROR_PRTF ("ERROR parse_http_header(): fields calloc() (ERRNO %d:%s)\n", errno, strerror(errno));
        free_http (http);
        return NULL;
    }
    memset (http->fields, 0, http->max_field_count * sizeof(http_field_t));
    return http;
}

http_t *init_http_with_arg (char *method, char *path, char *version, char *status)
{
    if (version == NULL || status == NULL)
    {
        ERROR_PRTF ("ERROR init_http_response(): NULL parameter\n");
        return NULL;
    }

    http_t *response = init_http();
    if (response == NULL)
    {
        ERROR_PRTF ("ERROR init_http_response(): init_http()\n");
        return NULL;
    }

    if (method)
    {
        response->method = copy_string(method);
        if (response->method == NULL)
        {
            ERROR_PRTF ("ERROR init_http_response(): method copy\n");
            goto ERROR;
        }
    }
    if (path)
    {
        response->path = copy_string(path);
        if (response->path == NULL)
        {
            ERROR_PRTF ("ERROR init_http_response(): path copy\n");
            goto ERROR;
        }
    }
    if (version)
    {
        response->version = copy_string(version);
        if (response->version == NULL)
        {
            ERROR_PRTF ("ERROR init_http_response(): version copy\n");
            goto ERROR;
        }
    }
    if (status)
    {
        response->status = copy_string(status);
        if (response->status == NULL)
        {
            ERROR_PRTF ("ERROR init_http_response(): status copy\n");
            goto ERROR;
        }
    }

    return response;
    ERROR:
    free_http(response);
    return NULL;
}

http_t *copy_http (http_t *http)
{
    if (http == NULL)
    {
        ERROR_PRTF ("ERROR copy_http(): NULL parameter\n");
        return NULL;
    }

    http_t *copy = init_http_with_arg (http->method, http->path, http->version, http->status);
    if (copy == NULL)
    {
        ERROR_PRTF ("ERROR copy_http(): init_http_with_arg()\n");
        return NULL;
    }

    if (add_body_to_http (copy, http->body_size, http->body_data) == -1)
    {
        ERROR_PRTF ("ERROR copy_http(): add_body_to_http()\n");
        goto ERROR;
    }

    for (int i = 0; i < http->field_count; i++)
    {
        if (add_field_to_http (copy, http->fields[i].field, http->fields[i].val) == -1)
        {
            ERROR_PRTF ("ERROR copy_http(): add_field_to_http()\n");
            goto ERROR;
        }
    }

    return copy;
    ERROR:
    free_http(copy);
    return NULL;
}

void free_http (http_t *http)
{
    if (http == NULL)
        return;
    free(http->method);
    free(http->path);
    free(http->version);
    free(http->status);
    free(http->body_data);
    if (http->fields)
    {
        for (int i = 0; i < http->max_field_count; i++)
        {
            free(http->fields[i].field);
            free(http->fields[i].val);
        }
        free (http->fields);
    }
    free(http);
}

char *find_http_field_val (http_t *http, char *field)
{
    if (http == NULL || field == NULL)
    {
        ERROR_PRTF ("ERROR find_http_request_header_field(): NULL parameter\n");
        return NULL;
    }
    for (int i = 0; i < http->field_count; i++)
    {
        if (strcmp(http->fields[i].field, field) == 0)
            return http->fields[i].val;
    }
    return NULL;
}

int add_field_to_http (http_t *http, char *field, char *val)
{
    if (http == NULL || field == NULL || val == NULL)
    {
        ERROR_PRTF ("ERROR add_field_to_http(): NULL parameter\n");
        return -1;
    }

    if (find_http_field_val (http, field) != NULL)
    {
        ERROR_PRTF ("ERROR add_field_to_http(): field already exists\n");
        return -1;
    }

    if (http->field_count + 1> http->max_field_count)
    {
        http->max_field_count *= 2;
        http->fields = (http_field_t *) realloc(http->fields, http->max_field_count * sizeof(http_field_t));
        if (http->fields == NULL)
        {
            ERROR_PRTF ("ERROR add_field_to_http(): fields realloc() (ERRNO %d:%s)\n", errno, strerror(errno));
            http->max_field_count /= 2;
            return -1;
        }
        memset (http->fields + http->field_count, 0, http->max_field_count / 2 * sizeof(http_field_t));
    }

    http->fields[http->field_count].field = copy_string (field);
    if (http->fields[http->field_count].field == NULL)
    {
        ERROR_PRTF ("ERROR add_field_to_http(): field copy\n");
        return -1;
    }

    http->fields[http->field_count].val = copy_string (val);
    if (http->fields[http->field_count].val == NULL)
    {
        ERROR_PRTF ("ERROR add_field_to_http(): val copy\n");
        return -1;
    }

    http->field_count++;
    return 0;
}

int remove_field_from_http (http_t *http, char *field)
{
    if (http == NULL || field == NULL)
    {
        ERROR_PRTF ("ERROR remove_field_from_http(): NULL parameter\n");
        return -1;
    }

    if (http->field_count == 0)
        return 0;

    int idx = -1;
    for (int i = 0; i < http->field_count; i++)
    {
        if (strcmp(http->fields[i].field, field) == 0)
        {
            idx = i;
            break;
        }
    }
    if (idx == -1)
        return 0;

    free(http->fields[idx].field);
    free(http->fields[idx].val);
    for (int i = idx; i < http->field_count - 1; i++)
    {
        http->fields[i].field = http->fields[i + 1].field;
        http->fields[i].val = http->fields[i + 1].val;
    }
    http->field_count--;
    return 0;
}

int add_body_to_http (http_t *http, size_t body_size, void *body_data)
{
    if (http == NULL)
    {
        ERROR_PRTF ("ERROR add_body_to_http_response(): NULL parameter\n");
        return -1;
    }
    if (body_size == 0 || body_data == NULL)
        return 0;

    if (http->body_data != NULL || http->body_size != 0)
    {
        ERROR_PRTF ("ERROR add_body_to_http_response(): body_data already exists\n");
        return -1;
    }

    if (find_http_field_val (http, "Content-Length") == NULL)
    {
        char content_length[32] = {0};
        sprintf(content_length, "%lu", body_size);
        if (add_field_to_http (http, "Content-Length", content_length) == -1)
        {
            ERROR_PRTF ("ERROR add_body_to_http_response(): add_field_to_http()\n");
            return -1;
        }
    }

    http->body_size = body_size;
    http->body_data = (void *) malloc(body_size);
    if (http->body_data == NULL)
    {
        ERROR_PRTF ("ERROR add_body_to_http_response(): body_data malloc()\n");
        if (remove_field_from_http (http, "Content-Length") == -1)
            ERROR_PRTF ("ERROR add_body_to_http_response(): remove_field_from_http()\n");
        return -1;
    }
    memcpy(http->body_data, body_data, body_size);

    return 0;
}

int remove_body_from_http (http_t *http)
{
    if (http == NULL)
    {
        ERROR_PRTF ("ERROR remove_body_from_http_response(): NULL parameter\n");
        return -1;
    }

    if (http->body_data == NULL || http->body_size == 0)
        return 0;

    free(http->body_data);
    http->body_data = NULL;
    http->body_size = 0;

    if (remove_field_from_http (http, "Content-Length") == -1)
    {
        ERROR_PRTF ("ERROR remove_body_from_http_response(): remove_field_from_http()\n");
        return -1;
    }

    return 0;
}

ssize_t write_http_to_buffer (http_t *http, void** buffer_ptr)
{
    size_t buffer_size = 0;
    if (http == NULL || buffer_ptr == NULL)
    {
        ERROR_PRTF ("ERROR write_http_to_buffer(): NULL parameter\n");
        return -1;
    }
    if (http->version == NULL)
    {
        ERROR_PRTF ("ERROR write_http_to_buffer(): NULL version\n");
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
    *buffer_ptr = (void *) calloc(1, buffer_size);
    if (*buffer_ptr == NULL)
    {
        ERROR_PRTF ("ERROR write_http_to_buffer(): buffer calloc() (ERRNO %d:%s)\n", errno, strerror(errno));
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
        ERROR_PRTF ("ERROR write_http_to_buffer(): buffer size\n");
        free (*buffer_ptr);
        *buffer_ptr = NULL;
        return -1;
    }       
    if (http->body_data)
    {
        memcpy(buffer, http->body_data, http->body_size);
        buffer += http->body_size;
    }
    return buffer_size;
}

/// UTILITIES ///

void print_tuple_yellow (char *a, char *b, int format_len, int indent)
{
    for (int i = 0; i < indent; i++)
        printf ("\t");
    YELLOW_PRTF ("%s: %*s", a? a : "NULL", format_len - (int)strlen(a), "");
    printf ("%s\n", b? b : "NULL");
}

void print_tuple_STATUS (char *a, char *status, int format_len, int indent)
{
    for (int i = 0; i < indent; i++)
        printf ("\t");
    YELLOW_PRTF ("%s: %*s", a? a : "NULL", format_len - (int)strlen(a), "");
    printf ("%s", status == NULL? "" : atoi(status) >= 400? "\033[1;31m" : 
        atoi(status) >= 300 ? "\033[1;33m" : "\033[1;32m");
    printf ("%s\n", status? status : "NULL");
    printf ("\033[0m");
}

void print_http_header (http_t *http)
{
    if (http == NULL)
    {
        ERROR_PRTF ("ERROR print_http_header(): NULL parameter\n");
        return;
    }
    int longest_element = 11;
    int longest_field = 0;
    for (int i = 0; i < http->field_count; i++)
        if (strlen(http->fields[i].field) > longest_field)
            longest_field = strlen(http->fields[i].field);

    print_tuple_yellow ("METHOD", http->method, longest_element, 2);
    print_tuple_yellow ("PATH", http->path, longest_element, 2);
    print_tuple_yellow ("VERSION", http->version, longest_element, 2);
    print_tuple_STATUS ("STATUS", http->status, longest_element, 2);
    char int_str[32] = {0};
    sprintf(int_str, "%lu", http->body_size);
    print_tuple_yellow ("BODY SIZE", int_str, longest_element, 2);
    sprintf(int_str, "%d", http->field_count);
    print_tuple_yellow ("FIELD COUNT", (char *) int_str, longest_element, 2);
    YELLOW_PRTF ("\t\tFIELDS:\n");
    for (int i = 0; i < http->field_count; i++)
        print_tuple_yellow (http->fields[i].field, http->fields[i].val, longest_field, 3);
}

void print_with_r_n (char *str)
{
    if (str == NULL)
    {
        ERROR_PRTF ("ERROR print_with_r_n(): NULL parameter\n");
        return;
    }
    int i = 0;
    for (; i < strlen(str); i++)
    {
        if (str[i] == '\r' && str[i + 1] == '\n')
            printf ("\\r\\n\r");
        else
            printf ("%c", str[i]);
    }
    if (str[i - 1] == '\r')
        printf ("\\r\\n");
    printf ("\n");

    fflush (stdout);
}

char *base64_encode(char *data, size_t input_length) 
{
    static char encoding_table[] = {
                                'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H',
                                'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
                                'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X',
                                'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f',
                                'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n',
                                'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
                                'w', 'x', 'y', 'z', '0', '1', '2', '3',
                                '4', '5', '6', '7', '8', '9', '+', '/'};
    static int mod_table[] = {0, 2, 1};

    size_t output_length = 4 * ((input_length + 2) / 3);

    char *encoded_data = calloc(1, output_length);
    if (encoded_data == NULL)
    {
        ERROR_PRTF ("ERROR base64_encode(): calloc() failed.\n");
        return NULL;
    }

    for (int i = 0, j = 0; i < input_length;) {

        uint32_t octet_a = i < input_length ? (unsigned char)data[i++] : 0;
        uint32_t octet_b = i < input_length ? (unsigned char)data[i++] : 0;
        uint32_t octet_c = i < input_length ? (unsigned char)data[i++] : 0;

        uint32_t triple = (octet_a << 0x10) + (octet_b << 0x08) + octet_c;

        encoded_data[j++] = encoding_table[(triple >> 3 * 6) & 0x3F];
        encoded_data[j++] = encoding_table[(triple >> 2 * 6) & 0x3F];
        encoded_data[j++] = encoding_table[(triple >> 1 * 6) & 0x3F];
        encoded_data[j++] = encoding_table[(triple >> 0 * 6) & 0x3F];
    }

    for (int i = 0; i < mod_table[input_length % 3]; i++)
        encoded_data[output_length - 1 - i] = '=';

    encoded_data [output_length] = '\0';
    return encoded_data;
}

char *copy_string (char *string)
{
    if (string == NULL)
    {
        ERROR_PRTF ("ERROR copy_string(): NULL parameter\n");
        return NULL;
    }
    char *copy = (char *) calloc(strlen(string) + 1, sizeof(char));
    if (copy == NULL)
    {
        ERROR_PRTF ("ERROR copy_string(): calloc() (ERRNO %d:%s)\n", errno, strerror(errno));
        return NULL;
    }
    strcpy(copy, string);
    return copy;
}

ssize_t write_bytes (int socket, void *buffer, size_t size)
{
    ssize_t bytes_sent = 0;
    ssize_t bytes_remaining = size;
    signal(SIGPIPE, SIG_IGN);
    while (bytes_remaining > 0)
    {
        bytes_sent = write(socket, buffer, bytes_remaining);
        if (bytes_sent == -1)
        {
            ERROR_PRTF ("ERROR write_bytes(): write() failed. (ERRNO %d:%s)\n", errno, strerror(errno));
            return -1;
        }
        bytes_remaining -= bytes_sent;
        buffer += bytes_sent;
    }
    return size;
}

ssize_t read_bytes (int socket, void *buffer, size_t size)
{
    ssize_t bytes_received = 0;
    ssize_t bytes_remaining = size;
    while (bytes_remaining > 0)
    {
        bytes_received = read(socket, buffer, bytes_remaining);
        if (bytes_received == -1)
        {
            ERROR_PRTF ("ERROR read_bytes(): read() failed. (ERRNO %d:%s)\n", errno, strerror(errno));
            return -1;
        }
        bytes_remaining -= bytes_received;
        buffer += bytes_received;
    }
    return size;
}

ssize_t read_file (void** output, char *file_path)
{
    if (output == NULL || file_path == NULL)
    {
        ERROR_PRTF ("ERROR read_file(): NULL parameter\n");
        return -1;
    }

    FILE *fp = fopen(file_path, "rb");
    if (fp == NULL)
    {
        ERROR_PRTF ("ERROR read_file(): file does not exist. (ERRNO %d:%s)\n", errno, strerror(errno));
        return -1;
    }

    fseek(fp, 0, SEEK_END);
    size_t file_size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    if (file_size == 0)
    {
        if (*output != NULL)
            free (*output);
        *output = NULL;
        fclose(fp);
        return 0;
    }

    if (*output != NULL)
        free (*output);
    *output = (void *) malloc(file_size);
    if (*output == NULL)
    {
        ERROR_PRTF ("ERROR read_file(): malloc() (ERRNO %d:%s)\n", errno, strerror(errno));
        fclose(fp);
        return -1;
    }

    if (fread(*output, 1, file_size, fp) != file_size)
    {
        ERROR_PRTF ("ERROR read_file(): fread() (ERRNO %d:%s)\n", errno, strerror(errno));
        fclose(fp);
        free (*output);
        return -1;
    }

    fclose(fp);
    return file_size;
}

ssize_t write_file (char *file_path, void *data, size_t size)
{
    if (data == NULL || file_path == NULL)
    {
        ERROR_PRTF ("ERROR write_file(): NULL parameter\n");
        return -1;
    }

    FILE *fp = fopen(file_path, "wb");
    if (fp == NULL)
    {
        ERROR_PRTF ("ERROR write_file(): fopen() (ERRNO %d:%s)\n", errno, strerror(errno));
        return -1;
    }

    if (size == 0)
    {
        fclose(fp);
        return 0;
    }

    if (fwrite(data, 1, size, fp) != size)
    {
        ERROR_PRTF ("ERROR write_file(): fwrite() (ERRNO %d:%s)\n", errno, strerror(errno));
        fclose(fp);
        return -1;
    }

    fclose(fp);
    return size;
}

ssize_t append_file (char *file_path, void *data, size_t size)
{
    if (data == NULL || file_path == NULL)
    {
        ERROR_PRTF ("ERROR append_file(): NULL parameter\n");
        return -1;
    }

    FILE *fp = fopen(file_path, "ab");
    if (fp == NULL)
    {
        ERROR_PRTF ("ERROR append_file(): fopen() (ERRNO %d:%s)\n", errno, strerror(errno));
        return -1;
    }

    if (size == 0)
    {
        fclose(fp);
        return 0;
    }

    if (fwrite(data, 1, size, fp) != size)
    {
        ERROR_PRTF ("ERROR append_file(): fwrite() (ERRNO %d:%s)\n", errno, strerror(errno));
        fclose(fp);
        return -1;
    }
    fclose(fp);
    return size;
}

char *get_file_extension (char *file_path)
{
    if (file_path == NULL)
    {
        ERROR_PRTF ("ERROR get_file_extension(): NULL parameter\n");
        return NULL;
    }
    char *extension = strrchr(file_path, '.');
    if (extension == NULL)
        return NULL;
    return extension + 1;
}
