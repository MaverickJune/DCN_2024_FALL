// NXC Data Communications Network http_engine.c for HTTP server
// Written by Jongseok Park (cakeng@snu.ac.kr)
// 2023. 9. 11

#include "http_functions.h"
#include "sys/socket.h"
#include "arpa/inet.h"
#include "netinet/tcp.h"

#define MAX_WAITING_CONNECTIONS 10 // Maximum number of waiting connections
#define MAX_PATH_SIZE 256 // Maximum size of path
#define SERVER_ROOT "./server_root"
#define ALBUM_PATH "/public/album"
#define ALBUM_HTML_PATH "./server_root/public/album/album_images.html"
#define ALBUM_HTML_TEMPLATE "<div class=\"card\"> <img src=\"/public/album/%s\" alt=\"Unable to load %s\"> </div>\n"


http_t *parse_http_header_ans (char *header_str)
{
    if (header_str == NULL)
    {
        ERROR_PRTF ("ERROR parse_http_header(): NULL parameter\n");
        return NULL;
    }

    http_t *http = init_http();

    char *line = strtok(header_str, "\r\n");
    char *next_line = line + strlen(line) + 1;
    if (line == NULL)
    {
        ERROR_PRTF ("ERROR parse_http_header(): \\r\\n not found.\n");
        free_http(http);
        return NULL;
    }
    char *token = strtok (line, " ");
    while (token != NULL)
    {
        if (http->method == NULL)
        {
            http->method = copy_string(token);
            if (http->method == NULL)
            {
                ERROR_PRTF ("ERROR parse_http_header(): method copy\n");
                free_http(http);
                return NULL;
            }
        }
        else if (http->path == NULL)
        {
            // Truncate query string and fragment identifier
            char *query_string = strchr(token, '?');
            if (query_string != NULL)
                *query_string = '\0';
            char *fragment_identifier = strchr(token, '#');
            if (fragment_identifier != NULL)
                *fragment_identifier = '\0';
            http->path = copy_string(token);
            if (http->path == NULL)
            {
                ERROR_PRTF ("ERROR parse_http_header(): path copy\n");
                free_http(http);
                return NULL;
            }
        }
        else if (http->version == NULL)
        {
            http->version = copy_string(token);
            if (http->version == NULL)
            {
                ERROR_PRTF ("ERROR parse_http_header(): version copy\n");
                free_http(http);
                return NULL;
            }
        }
        else
        {
            ERROR_PRTF ("ERROR parse_http_header(): first-line token - token: %s\n", token);
            free_http(http);
            return NULL;
        }
        token = strtok (NULL, " ");
    }
    if (http->method == NULL || http->path == NULL || http->version == NULL)
    {
        ERROR_PRTF ("ERROR parse_http_header(): first-line - method: %s, path: %s, version: %s\n",
                http->method? http->method : "NULL", http->path? http->path : "NULL", http->version? http->version : "NULL");
        free_http(http);
        return NULL;
    }

    line = strtok(next_line, "\r\n");
    if (line != NULL)
    {
        next_line = line + strlen(line) + 1;
        while (1)
        {
            token = strtok (line, ":");
            if (token == NULL)
            {
                ERROR_PRTF ("ERROR parse_http_header(): field token - token: %s\n", token? token : "NULL");
                free_http(http);
                return NULL;
            }
            char *field = token;
            char *val = strtok (NULL, "\r\n");
            while (val[0] == ' ')
                val++;
            if (add_field_to_http (http, field, val) == -1)
            {
                ERROR_PRTF ("ERROR parse_http_header(): add_field_to_http()\n");
                free_http(http);
                return NULL;
            }
            line = strtok(next_line, "\r\n");
            if (line == NULL)
                break;
            next_line = line + strlen(line) + 1;
        }
    }
    return http;
}

http_t *parse_multipart_content_body_ans (char** body_p, char* boundary, size_t body_size)
{
    if (body_p == NULL || boundary == NULL || body_size == 0 || *body_p == NULL)
    {
        ERROR_PRTF ("ERROR parse_multipart_content_body(): NULL parameter\n");
        return NULL;
    }
    char *body = *body_p;
    char *head_ptr = body, *eoh_ptr = body;
    while (1)
    {
        if (strncmp (head_ptr, boundary, strlen(boundary)) == 0)
        {
            head_ptr += strlen(boundary);
            if (strncmp (head_ptr, "--", 2) == 0)
            {
                *body_p = body + body_size;
                return NULL;
            }
            head_ptr += 2;
            break;
        }
        head_ptr++;
        if (head_ptr - body >= body_size)
        {
            ERROR_PRTF ("ERROR parse_multipart_content_body(): boundary not found\n");
            return NULL;
        }
    }

    eoh_ptr = head_ptr;
    while (strncmp (eoh_ptr, "\r\n\r\n", 4) != 0)
    {
        eoh_ptr++;
        if (eoh_ptr - body >= body_size)
        {
            ERROR_PRTF ("ERROR parse_multipart_content_body(): end of header not found\n");
            return NULL;
        }
    }
    eoh_ptr += 4;
    *(eoh_ptr - 1) = '\0';

    http_t *http = init_http();
    if (http == NULL)
    {
        ERROR_PRTF ("ERROR parse_multipart_content_body(): init_http()\n");
        return NULL;
    }

    char *line = strtok(head_ptr, "\r\n");
    char *next_line = line + strlen(line) + 1;
    while (1)
    {
        char *token = strtok (line, ":");
        if (token == NULL)
        {
            ERROR_PRTF ("ERROR parse_multipart_content_body(): field token - token: %s\n", token? token : "NULL");
            free_http(http);
            return NULL;  
        }
        char *field = token;
        char *val = strtok (NULL, "\r\n");
        while (val[0] == ' ')
            val++;
        if (add_field_to_http (http, field, val) == -1)
        {
            ERROR_PRTF ("ERROR parse_multipart_content_body(): add_field_to_http()\n");
            free_http(http);
            return NULL;  
        }
        line = strtok(next_line, "\r\n");
        if (line == NULL)
            break;
        next_line = line + strlen(line) + 1;
    }

    head_ptr = eoh_ptr;
    while (1)
    {
        if (head_ptr - body >= body_size || strncmp (head_ptr, boundary, strlen(boundary)) == 0)
            break;
        head_ptr++;
    }
    head_ptr -= 4; // \r\n--

    size_t data_size = head_ptr - eoh_ptr;
    if (add_body_to_http (http, data_size, eoh_ptr) == -1)
    {
        ERROR_PRTF ("ERROR parse_multipart_content_body(): add_body_to_http()\n");
        free_http(http);
        return NULL;    
    }

    *body_p = head_ptr;
    return http;
}


// Server routine for HTTP/1.0.
// Returns -1 if error occurs, 0 otherwise.
int server_routine_ans (int client_sock)
{
    if (client_sock == -1)
        return -1;
        
    size_t bytes_received = 0;
    char *http_version = "HTTP/1.0";
    char header_buffer[MAX_HTTP_MSG_HEADER_SIZE] = {0};
    char *eoh_ptr = header_buffer;
    int header_too_large_flag = 0;

    // Receive the HEADER of the client http message.
    // You have to consider the following cases:
    // 1. End of header (eoh) indicator of http is received (i.e., \r\n\r\n is received)
    // 2. Error occurs on read() (i.e. read() returns -1)
    // 3. Client disconnects (i.e. read() returns 0)
    // 4. MAX_HTTP_MSG_HEADER_SIZE is reached (i.e. message is too long)
    while (1)
    {
        // Receive the header of the client http message.
        ssize_t num_bytes = read(client_sock, header_buffer + bytes_received, MAX_HTTP_MSG_HEADER_SIZE - bytes_received);
        bytes_received += num_bytes;
        if (num_bytes == -1)
        {
            ERROR_PRTF ("SERVER ERROR: read() error (ERRNO %d:%s)\n", errno, strerror(errno));
            return -1;
        }
        else if (num_bytes == 0)
        {
            ERROR_PRTF ("SERVER ERROR:Client disconnected\n");
            return -1;
        }
        else
        {
            int eoh_found = 0;
            for (; eoh_ptr - header_buffer < bytes_received - 3; eoh_ptr++)
            {
                if (strncmp (eoh_ptr, "\r\n\r\n", 4) == 0)
                {
                    eoh_ptr += 4;
                    eoh_found = 1;
                    break;
                }
            }
            if (eoh_found)
            {
                *(eoh_ptr - 1) = '\0';
                break;
            }
            else if (bytes_received >= MAX_HTTP_MSG_HEADER_SIZE)
            {
                ERROR_PRTF ("SERVER ERROR: Header too large\n");
                header_too_large_flag = 1;
                break;
            }
        }
    }
    http_t *request = NULL, *response = NULL;

    // Create different http response depending on the request.
    // Case 1: Request Header Fields Too Large
    // In most real-world web browsers, this error rarely occurs.
    // However, we included this case to give you a hint on how to handle other errors.
    if (header_too_large_flag)
    {
        char body[] = "<html><body><h1>431 Request Header Fields Too Large</h1></body></html>";
        response = init_http_with_arg (NULL, NULL, http_version, "431");
        if (response == NULL)
        {
            ERROR_PRTF ("SERVER ERROR: Failed to create HTTP response\n");
            return -1;
        }
        add_body_to_http (response, sizeof(body), body);
        add_field_to_http (response, "Content-Type", "text/html");
        add_field_to_http (response, "Connection", "close");
    }
    else
    {
        // Parse the header of the client http message.
        request = parse_http_header_ans (header_buffer);
        if (request == NULL)
        {
            ERROR_PRTF ("SERVER ERROR: Failed to parse HTTP header\n");
            free_http (request);
            free_http (response);
            return -1;
        }
        printf ("\tHTTP ");
        GREEN_PRTF ("REQUEST:\n");
        print_http_header (request);

        // Case 2: GET request
        if (strncmp (request->method, "GET", 3) == 0)
        {
            // Get /index.html when / is requested.
            if (strncmp (request->path, "/", 2) == 0)
            {
                free (request->path);
                request->path = copy_string ("/index.html");
            }
            // Check if the requested file needs authorization.
            // ID and password are "DCN" and "FALL2023"
            // Please do not change the ID and password when submitting.
            int auth_flag = 0;
            char *auth_list[] = {"/secret.html", "/public/images/khl.jpg"};
            char ans_plain[] = "DCN:FALL2023";

            // Check if the requested file is in the auth_list. If so, set auth_flag to 1.
            for (int i = 0; i < sizeof(auth_list)/sizeof(char *); i++)
            {
                if (strncmp (request->path, auth_list[i], strlen(auth_list[i])) == 0)
                {
                    auth_flag = 1;
                    break;
                }
            }

            // Check if the auth_flag is set to 1 and the request has Authorization field.
            // If so, check if the ID and password are correct. If correct, set auth_flag to 0.
            if (auth_flag == 1 && find_http_field_val (request, "Authorization"))
            {
                char *auth = find_http_field_val (request, "Authorization");
                char *auth_base64 = strrchr (auth, ' ') + 1;
                char *ans_base64 = base64_encode (ans_plain, strlen (ans_plain));
                if (strncmp (auth_base64, ans_base64, strlen (ans_base64)) == 0)
                    auth_flag = 0;
                free (ans_base64);
            }
                
            // Case 2-1: If authorization succeeded...
            if (auth_flag == 0)
            {
                // Get the file path from the request.
                // Case 2-1-1: If the file does not exist, send 404 Not Found.
                // Case 2-1-2: If the file exists, send 200 OK with the file as the body.

                char file_path[MAX_PATH_SIZE] = {0};
                void *file_buffer = NULL;
                ssize_t file_size = -1;

                snprintf (file_path, MAX_PATH_SIZE, "%s%s", SERVER_ROOT, request->path);
                file_size = read_file (&file_buffer, file_path);
                if (file_size == -1)
                {
                    
                    char body[] = "<html><body><h1>404 Not Found</h1></body></html>";
                    response = init_http_with_arg (NULL, NULL, http_version, "404");
                    if (response == NULL)
                    {
                        ERROR_PRTF ("SERVER ERROR: Failed to create HTTP response\n");
                        free_http (request);
                        free_http (response);
                        return -1;
                    }
                    add_body_to_http (response, sizeof(body), body);
                    add_field_to_http (response, "Content-Type", "text/html");
                    add_field_to_http (response, "Connection", "close");

                }
                else
                {
                    
                    response = init_http_with_arg (NULL, NULL, http_version, "200");
                    if (response == NULL)
                    {
                        ERROR_PRTF ("SERVER ERROR: Failed to create HTTP response\n");
                        free (file_buffer);
                        free_http (request);
                        free_http (response);
                        return -1;
                    }
                    add_body_to_http (response, file_size, file_buffer);
                    add_field_to_http (response, "Connection", "close");
                    free (file_buffer);
                    if (strncmp (get_file_extension (file_path), "html", 4) == 0)
                        add_field_to_http (response, "Content-Type", "text/html");
                    else if (strncmp (get_file_extension (file_path), "css", 3) == 0)
                        add_field_to_http (response, "Content-Type", "text/css");
                    else if (strncmp (get_file_extension (file_path), "js", 2) == 0)
                        add_field_to_http (response, "Content-Type", "text/javascript");
                    else if (strncmp (get_file_extension (file_path), "png", 3) == 0)
                        add_field_to_http (response, "Content-Type", "image/png");
                    else if (strncmp (get_file_extension (file_path), "jpg", 3) == 0)
                        add_field_to_http (response, "Content-Type", "image/jpeg");
                    else
                        add_field_to_http (response, "Content-Type", "application/octet-stream");
                }
            }
            // Case 2-2: If authorization failed...
            else
            {
                // send 401 Unauthorized with WWW-Authenticate field.
                char body[] = "<html><body><h1>401 Unauthorized</h1></body></html>";
                response = init_http_with_arg (NULL, NULL, http_version, "401");
                if (response == NULL)
                {
                    ERROR_PRTF ("SERVER ERROR: Failed to create HTTP response\n");
                    free_http (request);
                    free_http (response);
                    return -1;
                }
                add_body_to_http (response, sizeof(body), body);
                add_field_to_http (response, "Content-Type", "text/html");
                add_field_to_http (response, "Connection", "close");
                add_field_to_http (response, "WWW-Authenticate", "Basic realm=\"ID & Password?\"");
            }
        }
        else if (strncmp (request->method, "POST", 4) == 0)
        {
            // Case 3: POST request
            // Refer to https://developer.mozilla.org/en-US/docs/Web/HTTP/Methods/POST
            char *body_len_str = find_http_field_val (request, "Content-Length");
            size_t body_len = atoi (body_len_str);
            // Assumes that the boundary is the last field value in Content-Type
            char *boundary = strstr (find_http_field_val (request, "Content-Type"), "boundary=") + 9;
            char *request_body = (char *)calloc (1, body_len + 1);
            if (request_body == NULL)
            {
                ERROR_PRTF ("SERVER ERROR: Failed to allocate memory for request_body\n");
                free_http (request);
                free_http (response);
                return -1;
            }
            // Read the request_body of the client http message.
            size_t body_in_header_len = bytes_received - (eoh_ptr - header_buffer);
            if (body_in_header_len > 0)
                memcpy (request_body, eoh_ptr, body_in_header_len);
            read_bytes (client_sock, request_body + body_in_header_len, body_len - body_in_header_len);

            char *body_ptr = request_body;
            while (1)
            {
                // Parse the request_body of the multipart content request_body.
                http_t *body_part = parse_multipart_content_body_ans (&body_ptr, boundary, 
                    (request_body + body_len) - body_ptr);
                if (body_ptr >= request_body + body_len)
                    break;
                if (body_part == NULL)
                {
                    ERROR_PRTF ("SERVER ERROR: Failed to parse multipart content request body\n");
                    free (request_body);
                    free_http (request);
                    free_http (response);
                    return -1;
                }
                printf ("\tHTTP ");
                GREEN_PRTF ("POST BODY:\n");
                print_http_header (body_part);

                // Get the filename of the file.
                char *filename = strstr (find_http_field_val(body_part, "Content-Disposition"), "filename=");
                if (filename == NULL)
                {
                    ERROR_PRTF ("SERVER ERROR: Failed to get filename\n");
                    free (request_body);
                    free_http (body_part);
                    free_http (request);
                    free_http (response);
                    return -1;
                }
                filename += 10;
                strchr (filename, '"')[0] = '\0';
                
                // Check if the file is an image file.
                char *extension = get_file_extension (filename);
                if (strncmp (extension, "jpg", 3) != 0)
                {
                    ERROR_PRTF ("SERVER ERROR: Invalid file type\n");
                    free (request_body);
                    free_http (body_part);
                    free_http (request);
                    free_http (response);
                    return -1;
                }

                // Add the file to the album.
                char path[MAX_PATH_SIZE] = {0};
                snprintf (path, MAX_PATH_SIZE-1, "%s%s/%s", SERVER_ROOT, ALBUM_PATH, filename);
                path [MAX_PATH_SIZE-1] = '\0';
                if (write_file (path, body_part->body_data, body_part->body_size) == -1)
                {
                    ERROR_PRTF ("SERVER ERROR: Failed to write file\n");
                    free (request_body);
                    free_http (body_part);
                    free_http (request);
                    free_http (response);
                    return -1;
                }

                // Append the appropriate html for the new image to album.html.
                size_t html_size = strlen (ALBUM_HTML_TEMPLATE) + strlen (filename)*2 + 1;
                char *html = (char *)calloc (1, html_size);
                if (html == NULL)
                {
                    ERROR_PRTF ("SERVER ERROR: Failed to allocate memory for html\n");
                    free (request_body);
                    free_http (request);
                    free_http (response);
                    return -1;
                }
                sprintf (html, ALBUM_HTML_TEMPLATE, filename, filename);
                append_file (ALBUM_HTML_PATH , html, strlen (html));
                free (html);
                free_http (body_part);
            }
            free (request_body);

            // Respond with a 200 OK.
            char body[] = "<html><body><h1>200 OK</h1></body></html>";
            response = init_http_with_arg (NULL, NULL, http_version, "200");
            if (response == NULL)
            {
                ERROR_PRTF ("SERVER ERROR: Failed to create HTTP response\n");
                free_http (request);
                free_http (response);
                return -1;
            }
            add_body_to_http (response, sizeof(body), body);
            add_field_to_http (response, "Content-Type", "text/html");
            add_field_to_http (response, "Connection", "close");
        }
        else
        {
            // Case 4: Other requests
            char body[] = "<html><body><h1>400 Bad Request</h1></body></html>";
            response = init_http_with_arg (NULL, NULL, http_version, "400");
            if (response == NULL)
            {
                ERROR_PRTF ("SERVER ERROR: Failed to create HTTP response\n");
                free_http (request);
                free_http (response);
                return -1;
            }
            add_body_to_http (response, sizeof(body), body);
            add_field_to_http (response, "Content-Type", "text/html");
            add_field_to_http (response, "Connection", "close");
        }
    }

    // Send the response to the client.
    if (response != NULL)
    {
        printf ("\tHTTP ");
        GREEN_PRTF ("RESPONSE:\n");
        print_http_header (response);

        // Parse http response to buffer
        void *response_buffer = NULL;
        ssize_t response_size = write_http_to_buffer (response, &response_buffer);
        if (response_size == -1)
        {
            ERROR_PRTF ("SERVER ERROR: Failed to write HTTP response to buffer\n");
            free_http (request);
            free_http (response);
            return -1;
        }

        // Send http response to client
        if (write_bytes (client_sock, response_buffer, response_size) == -1)
        {
            ERROR_PRTF ("SERVER ERROR: Failed to send response to client\n");
            free (response_buffer);
            free_http (request);
            free_http (response);
            return -1;
        }

        free (response_buffer);
    }
    free_http (request);
    free_http (response);
    return 0;
}


int server_engine_ans (int server_port)
{
    // Initialize server socket
    int server_listening_sock = socket(PF_INET, SOCK_STREAM, 0);
    int val = 1;
    setsockopt(server_listening_sock, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val));
    if (server_listening_sock == -1)
    {
        ERROR_PRTF ("SERVER ERROR: setsocketopt() SO_REUSEADDR error (ERRNO %d:%s)\n", errno, strerror(errno));
        return -1;
    }

    // Bind server socket to the given port
    struct sockaddr_in server_addr_info;
    memset(&server_addr_info, 0, sizeof(server_addr_info));
    server_addr_info.sin_family = AF_INET;
    server_addr_info.sin_port = htons(server_port);
    server_addr_info.sin_addr.s_addr = htonl(INADDR_ANY);
    if (bind(server_listening_sock, (struct sockaddr *)&server_addr_info, sizeof(server_addr_info)) == -1)
    {
        ERROR_PRTF ("SERVER ERROR: bind() error (ERRNO %d:%s)\n", errno, strerror(errno));;
        close (server_listening_sock);
        return -1;
    }

    // Listen for incoming connections
    if (listen(server_listening_sock, MAX_WAITING_CONNECTIONS) == -1)
    {
        ERROR_PRTF ("SERVER ERROR: listen() error (ERRNO %d:%s)\n", errno, strerror(errno));
        close (server_listening_sock);
        return -1;
    }

    // Serve incoming connections forever
    while (1)
    {
        struct sockaddr_in client_addr_info;
        socklen_t client_addr_info_len = sizeof(client_addr_info);
        int client_connected_sock;

        // Accept incoming connections
        client_connected_sock = accept(server_listening_sock, (struct sockaddr *)&client_addr_info, &client_addr_info_len);
        if (client_connected_sock == -1)
        {
            ERROR_PRTF ("SERVER ERROR: Failed to accept an incoming connection (ERRNO %d:%s)\n", errno, strerror(errno));
            continue;
        }
        printf ("CLIENT ");
        printf("%s:%d ", inet_ntoa(client_addr_info.sin_addr), ntohs(client_addr_info.sin_port));
        GREEN_PRTF ("CONNECTED.\n");

        // Serve the client
        server_routine_ans (client_connected_sock);
        
        // Close the connection with the client
        close(client_connected_sock);
        printf ("CLIENT ");
        printf ("%s:%d ", inet_ntoa(client_addr_info.sin_addr), ntohs(client_addr_info.sin_port));
        GREEN_PRTF ("DISCONNECTED.\n\n");
        fflush (stdout);
    }
    // Close the server socket
    close(server_listening_sock);
    return 0;
}