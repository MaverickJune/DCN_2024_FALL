// NXC Data Communications http_server.c
// Written by Jongseok Park (cakeng@snu.ac.kr)
// 2023. 9. 11

#include "http_util.h"
#include "sys/socket.h"
#include "arpa/inet.h"
#include "netinet/tcp.h"

#define MAX_WAITING_CONNECTIONS 100 // Maximum number of waiting connections
#define SERVER_ROOT "./server_root"

int server_https_1_0_routine (int client_sock);
int server_https_1_1_routine (int client_sock);

int main (int argc, char **argv)
{
    // Parse inputs
    if (argc != 3) 
    {
        printf ("Usage: %s <port> <http_version>\n", argv[0]);
        printf ("ex) %s 62123 1.0\n", argv[0]);
        return 1;
    }
    int server_port = atoi(argv[1]);
    if (server_port <= 0 || server_port > 65535)
    {
        printf ("Invalid port number: %s\n", argv[1]);
        return 1;
    }
    int version = 10;
    if (strncmp (argv[2], "1.0", 3) == 0)
        version = 10;
    else if (strncmp (argv[2], "1.1", 3) == 0)
        version = 11;
    else
    {
        printf ("Invalid HTTP version: %s\n", argv[2]);
        return 1;
    }

    // Initialize server socket
    int server_listening_sock = socket(PF_INET, SOCK_STREAM, 0);
    int val = 1;
    setsockopt(server_listening_sock, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val));
    if (server_listening_sock == -1)
    {
        printf("setsocketopt() SO_REUSEADDR error\n");
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
        printf("bind() error\n");
        return -1;
    }

    // Listen for incoming connections
    if (listen(server_listening_sock, MAX_WAITING_CONNECTIONS) == -1)
    {
        printf("listen() error\n");
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
            printf("Error: Failed to accept an incoming connection\n");
            return -1;
        }
        printf("Client %s:%d connected\n", inet_ntoa(client_addr_info.sin_addr), ntohs(client_addr_info.sin_port));

        // Serve the client
        if (version == 10)
            server_https_1_0_routine(client_connected_sock);
        else if (version == 11)
            server_https_1_1_routine(client_connected_sock);
        
        // Close the connection with the client
        close(client_connected_sock);
    }
}

// Server routine for HTTP/1.0.
// Returns -1 if error occurs, 0 otherwise.
int server_https_1_0_routine (int client_sock)
{
    static int num = 0;
    size_t bytes_received = 0;
    char header_buffer[MAX_HTTP_MSG_HEADER_SIZE] = {0};
    int header_too_long_flag = 0;

    // Receive the HEADER of the client http message.
    // You have to consider the following cases:
    // 1. End of header indicator of http is received
    // 2. Error occurs on read() (i.e. read() returns -1)
    // 3. Client disconnects (i.e. read() returns 0)
    // 4. MAX_HTTP_MSG_HEADER_SIZE is reached (i.e. message is too long)
    while (1)
    {
        // TODO: Receive the header of the client http message.
        ssize_t num_bytes = read(client_sock, header_buffer + bytes_received, MAX_HTTP_MSG_HEADER_SIZE - bytes_received);
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
        else if (bytes_received + num_bytes >= MAX_HTTP_MSG_HEADER_SIZE)
        {
            header_too_long_flag = 1;
            break;
        }
        else
        {
            bytes_received += num_bytes;
            if (bytes_received >= 4 && strncmp((header_buffer + bytes_received) - 4, "\r\n\r\n", 4) == 0)
                break;
        }
    }

    printf ("Received message from client:\n%s", header_buffer);
    
    http_t *request_header = parse_http_header (header_buffer);
    if (request_header == NULL)
    {
        printf ("SERVER ERROR: Failed to parse HTTP header\n");
        return -1;
    }
    printf ("http request received from client:\n");
    print_http (request_header);

    num++;

    http_t *response;
    
    // Create different http response depending on the request.

    
    
    response = init_http_with_arg (NULL, NULL, "HTTP/1.0", "200 OK");
    if (response == NULL)
    {
        printf ("SERVER ERROR: Failed to initialize HTTP response\n");
        return -1;
    }
    if (add_field_to_http (response, "Content-Type", "text/html") == -1)
    {
        printf ("SERVER ERROR: Failed to add field to HTTP response\n");
        return -1;
    }
    char body[1024] = {0};
    sprintf (body, "<html><body><h1>Hello, World!</h1><p>Number of requests: %d</p></body></html>", num);
    if (add_body_to_http (response, sizeof(body), (void*)body) == -1)
    {
        printf ("SERVER ERROR: Failed to add body to HTTP response\n");
        return -1;
    }

    printf ("http response sent to client:\n");
    print_http (response);

    void *response_buffer = NULL;
    ssize_t response_size = write_http_to_buffer (response, &response_buffer);
    if (response_size == -1)
    {
        printf ("SERVER ERROR: Failed to write HTTP response to buffer\n");
        return -1;
    }

    // printf ("Raw response sent to client:\n");
    // printf ("%s\n", (char*)response_buffer);

    if (write_bytes (client_sock, response_buffer, response_size) == -1)
    {
        printf ("SERVER ERROR: Failed to send response to client\n");
        return -1;
    }

    free_http (response);
    free (response_buffer);
    return 0;
}

int server_https_1_1_routine (int client_sock)
{
    return 0;
}

