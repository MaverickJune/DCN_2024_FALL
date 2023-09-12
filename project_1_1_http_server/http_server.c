// NXC Data Communications http_server.c
// Written by Jongseok Park (cakeng@snu.ac.kr)
// 2023. 9. 11

#include "socket_util.h"
#include "http_util.h"

#define SERVER_ROOT "./server_root"
static int num = 0;

int server_routine (int client_sock)
{
    size_t bytes_received = 0;
    char msg_buffer[MAX_HTTP_MSG_HEADER_SIZE] = {0};
    char *msg_ptr = msg_buffer;
    int header_too_long_flag = 0;

    // Receive client request message header until 
    // 1. End of message header is received (i.e. "\r\n\r\n" is received)
    // 2. Error occurs (i.e. read() returns -1)
    // 3. Client disconnects (i.e. read() returns 0)
    // 4. MAX_HTTP_MSG_HEADER_SIZE is reached (i.e. message is too long)
    while (1)
    {
        // Receive message from client. 
        // read() returns -1 if error occurs, 0 if client disconnected, and number of bytes received if successful.
        ssize_t num_bytes = read(client_sock, msg_ptr, MAX_HTTP_MSG_HEADER_SIZE - bytes_received);
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
            msg_ptr += num_bytes;
            if (bytes_received >= 4 && 
                strncmp(msg_ptr - 4, "\r\n\r\n", 4) == 0)
                break;
        }
    }

    printf ("Received message from client:\n%s", msg_buffer);

    num++;

    http_t *response = init_http_response ("HTTP/1.0", "200 OK");
    if (response == NULL)
    {
        printf ("SERVER ERROR: Failed to initialize HTTP response\n");
        return -1;
    }
    if (add_field_to_http (&response, "Content-Type", "text/html") == -1)
    {
        printf ("SERVER ERROR: Failed to add field to HTTP response\n");
        return -1;
    }
    if (add_field_to_http (&response, "date", "Tue, 12 Sep 2023 17:02:53 GMT") == -1)
    {
        printf ("SERVER ERROR: Failed to add field to HTTP response\n");
        return -1;
    }
    char body[1024] = {0};
    sprintf (body, "<html><body><h1>Hello, World!</h1><p>Number of requests: %d</p></body></html>", num);
    if (add_body_to_http (&response, sizeof(body), (void*)body) == -1)
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

int main (int argc, char **argv)
{
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
    int server_listening_sock = server_init_tcp_socket(server_port);
    if (server_listening_sock == -1)
    {
        printf("Error: Failed to initialize server socket\n");
        return -1;
    }

    // Serve incoming connections forever
    while (1)
    {
        // Wait for incoming connections
        int client_connected_sock = server_accept_tcp_socket(server_listening_sock);
        if (client_connected_sock == -1)
        {
            printf("Error: Failed to accept incoming connection\n");
            return -1;
        }

        // Serve the client
        server_routine(client_connected_sock);
        
        close(client_connected_sock);
    }
}
