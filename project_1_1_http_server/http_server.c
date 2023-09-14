// NXC Data Communications http_server.c
// Written by Jongseok Park (cakeng@snu.ac.kr)
// 2023. 9. 11

#include "http_util.h"
#include "sys/socket.h"
#include "arpa/inet.h"
#include "netinet/tcp.h"

#define MAX_WAITING_CONNECTIONS 100 // Maximum number of waiting connections
#define MAX_PATH_SIZE 1024 // Maximum size of path
#define MAX_PARALLEL_CONNECTIONS 100 // Maximum number of parallel connections
#define SERVER_ROOT "./server_root"

int server_routine (int client_sock);

int main (int argc, char **argv)
{
    // Parse inputs
    if (argc != 2) 
    {
        printf ("Usage: %s <port>\n", argv[0]);
        printf ("ex) %s 62123\n", argv[0]);
        return 1;
    }
    int server_port = atoi(argv[1]);
    if (server_port <= 0 || server_port > 65535)
    {
        printf ("Invalid port number: %s\n", argv[1]);
        return 1;
    }

    // Initialize server socket
    int server_listening_sock = socket(PF_INET, SOCK_STREAM, 0);
    int val = 1;
    setsockopt(server_listening_sock, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val));
    if (server_listening_sock == -1)
    {
        printf("setsocketopt() SO_REUSEADDR error\n");
        return 1;
    }

    // Bind server socket to the given port
    struct sockaddr_in server_addr_info;
    memset(&server_addr_info, 0, sizeof(server_addr_info));
    server_addr_info.sin_family = AF_INET;
    server_addr_info.sin_port = htons(server_port);
    server_addr_info.sin_addr.s_addr = htonl(INADDR_ANY);
    if (bind(server_listening_sock, (struct sockaddr *)&server_addr_info, sizeof(server_addr_info)) == -1)
    {
        printf("bind() error\n");
        return 1;
    }

    // Listen for incoming connections
    if (listen(server_listening_sock, MAX_WAITING_CONNECTIONS) == -1)
    {
        printf("listen() error\n");
        return 1;
    }

    // Serve incoming connections forever
    while (1)
    {
        #pragma omp parallel for num_threads(MAX_PARALLEL_CONNECTIONS)
        for (int i = 0; i < MAX_PARALLEL_CONNECTIONS; i++)
        {
            struct sockaddr_in client_addr_info;
            socklen_t client_addr_info_len = sizeof(client_addr_info);
            int client_connected_sock;

            #pragma omp critical
            {
                // Accept incoming connections
                client_connected_sock = accept(server_listening_sock, (struct sockaddr *)&client_addr_info, &client_addr_info_len);
            }
            if (client_connected_sock == -1)
                printf("Error: Failed to accept an incoming connection\n");

            // Serve the client
            server_routine (client_connected_sock);
            
            // Close the connection with the client
            close(client_connected_sock);
        }
    }
    // Close the server socket
    close(server_listening_sock);
    return 0;
}

// Server routine for HTTP/1.0.
// Returns -1 if error occurs, 0 otherwise.
int server_routine (int client_sock)
{
    size_t bytes_received = 0;
    char *http_version = "HTTP/1.0";
    char header_buffer[MAX_HTTP_MSG_HEADER_SIZE] = {0};
    int header_too_large_flag = 0;

    // Receive the HEADER of the client http message.
    // You have to consider the following cases:
    // 1. End of header indicator of http is received
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
            printf("SERVER ERROR: read() error\n");
            return -1;
        }
        else if (num_bytes == 0)
        {
            printf("SERVER ERROR:Client disconnected\n");
            return -1;
        }
        else if (bytes_received >= MAX_HTTP_MSG_HEADER_SIZE)
        {
            header_too_large_flag = 1;
            break;
        }
        else if (strncmp (header_buffer + bytes_received - 4, "\r\n\r\n", 4) == 0)
            break;
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
            printf ("SERVER ERROR: Failed to create HTTP response\n");
            return -1;
        }
        add_body_to_http (response, sizeof(body), body);
        add_field_to_http (response, "Content-Type", "text/html");
        add_field_to_http (response, "Connection", "close");
    }
    else
    {
        // Parse the header of the client http message.
        request = parse_http_header (header_buffer);
        if (request == NULL)
        {
            printf ("SERVER ERROR: Failed to parse HTTP header\n");
            return -1;
        }
        // printf ("\tHTTP request received from client:\n");
        // print_http_header (request);

        // Case 2: GET request
        if (strncmp (request->method, "GET", 3) == 0)
        {
            if (strncmp (request->path, "/", 2) == 0)
                request->path = copy_string ("/index.html");

            char file_path[MAX_PATH_SIZE] = {0};
            snprintf (file_path, MAX_PATH_SIZE, "%s%s", SERVER_ROOT, request->path);
            void *file_buffer = NULL;
            ssize_t file_size = read_file (&file_buffer, file_path);
            if (file_size == -1)
            {
                // Case 2-1: File not found
                char body[] = "<html><body><h1>404 Not Found</h1></body></html>";
                response = init_http_with_arg (NULL, NULL, http_version, "404");
                if (response == NULL)
                {
                    printf ("SERVER ERROR: Failed to create HTTP response\n");
                    return -1;
                }
                add_body_to_http (response, sizeof(body), body);
                add_field_to_http (response, "Content-Type", "text/html");
                add_field_to_http (response, "Connection", "close");
            }
            else
            {
                // Case 2-2: File found
                response = init_http_with_arg (NULL, NULL, http_version, "200");
                if (response == NULL)
                {
                    printf ("SERVER ERROR: Failed to create HTTP response\n");
                    return -1;
                }
                add_body_to_http (response, file_size, file_buffer);
                add_field_to_http (response, "Connection", "close");
                free (file_buffer);
                if (strncmp (get_file_extension (file_path), "html", 4) == 0)
                    add_field_to_http (response, "Content-Type", "text/html");
                else if (strncmp (get_file_extension (file_path), "jpg", 3) == 0)
                    add_field_to_http (response, "Content-Type", "image/jpeg");
                else if (strncmp (get_file_extension (file_path), "png", 3) == 0)
                    add_field_to_http (response, "Content-Type", "image/png");
                else if (strncmp (get_file_extension (file_path), "mp3", 3) == 0)
                    add_field_to_http (response, "Content-Type", "audio/mpeg");
                else
                    add_field_to_http (response, "Content-Type", "application/octet-stream");
            }
        }
        else
        {
            // Case 3: Other requests
            char body[] = "<html><body><h1>400 Bad Request</h1></body></html>";
            response = init_http_with_arg (NULL, NULL, http_version, "400");
            if (response == NULL)
            {
                printf ("SERVER ERROR: Failed to create HTTP response\n");
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
        // printf ("\tHTTP response sent to client:\n");
        // print_http_header (response);

        // Parse http response to buffer
        void *response_buffer = NULL;
        ssize_t response_size = write_http_to_buffer (response, &response_buffer);
        if (response_size == -1)
        {
            printf ("SERVER ERROR: Failed to write HTTP response to buffer\n");
            return -1;
        }

        // Send http response to client
        if (write_bytes (client_sock, response_buffer, response_size) == -1)
        {
            printf ("SERVER ERROR: Failed to send response to client\n");
            return -1;
        }
        free (response_buffer);
    }

    free_http (request);
    free_http (response);
    return 0;
}
