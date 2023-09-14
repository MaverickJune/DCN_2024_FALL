// NXC Data Communications http_server.c
// Written by Jongseok Park (cakeng@snu.ac.kr)
// 2023. 9. 11

#include "http_util.h"
#include "sys/socket.h"
#include "arpa/inet.h"
#include "netinet/tcp.h"
#include "omp.h"

#define MAX_WAITING_CONNECTIONS 10 // Maximum number of waiting connections
#define MAX_PATH_SIZE 256 // Maximum size of path
#define SERVER_ROOT "./server_root"

int server_engine (int server_port);
int server_routine (int client_sock);

int server_engine (int server_port)
{
    // TODO: Initialize server socket
    int server_listening_sock, val;

    // TODO: Bind server socket to the given port
    struct sockaddr_in server_addr_info;

    // TODO: Listen for incoming connections


    // TODO: Serve incoming connections forever
    while (1)
    {
        struct sockaddr_in client_addr_info;
        socklen_t client_addr_info_len = sizeof(client_addr_info);
        int client_connected_sock = -1;

        // TODO: Accept incoming connections

        // Serve the client.
        server_routine (client_connected_sock);
        
        // TODO: Close the connection with the client

    }
    // TODO: Close the server socket

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
    http_t *request = NULL, *response = NULL;

    // TODO: Receive the HEADER of the client http message.
    // You have to consider the following cases:
    // 1. End of header indicator of http is received
    // 2. Error occurs on read() (i.e. read() returns -1)
    // 3. Client disconnects (i.e. read() returns 0)
    // 4. MAX_HTTP_MSG_HEADER_SIZE is reached (i.e. message is too long)
    while (1)
    {
        break;
    }

    // TODO: Create different http response depending on the request.

    // Case 1: Request Header Fields Too Large - return 431 Request Header Fields Too Large
    // This case rarely happens, but we have included its implementation as a hint.
    if (header_too_large_flag)
    {
        response = init_http_with_arg (NULL, NULL, http_version, "431");
        if (response == NULL)
        {
            printf ("SERVER ERROR: Failed to create HTTP response\n");
            return -1;
        }

        char body[] = "<html><body><h1>431 Request Header Fields Too Large</h1></body></html>";
        add_body_to_http (response, sizeof(body), body);
        add_field_to_http (response, "Content-Type", "text/html");

        // Make sure to send "Connection: close" field in the header. (HTTP/1.0) 
        add_field_to_http (response, "Connection", "close");

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
    }
    // TODO: Parse the header of the client http message.


    // TODO: Case 2: GET request - return 200 OK or 404 Not Found
        

    // TODO: Case 3: Other requests - Return 400 Bad Request


    // TODO: Free http structs

    return 0;
}