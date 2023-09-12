// NXC Data Communications http_server.c
// Written by Jongseok Park (cakeng@snu.ac.kr)
// 2023. 9. 11

#include "socket_util.h"


#define SERVER_ROOT "./server_root"

void server_routine (int client_sock)
{
    size_t bytes_received = 0;
    char msg_buffer[MAX_HTTP_GET_MSG_SIZE] = {0};
    char *msg_ptr = msg_buffer;

    // Receive client request message until 
    // 1. End of message is received (i.e. "\r\n\r\n" is received)
    // 2. Error occurs (i.e. read() returns -1)
    // 3. Client disconnects (i.e. read() returns 0)
    // 4. MAX_HTTP_GET_MSG_SIZE is reached (i.e. message is too long)
    while (bytes_received < MAX_HTTP_GET_MSG_SIZE)
    {
        // Receive message from client. 
        // read() returns -1 if error occurs, 0 if client disconnected, and number of bytes received if successful.
        ssize_t num_bytes = read(client_sock, msg_ptr, MAX_HTTP_GET_MSG_SIZE - bytes_received);
        if (num_bytes == -1)
            printf("read() error\n");
        else if (num_bytes == 0)
            printf("Client disconnected\n");
        else
        {
            bytes_received += num_bytes;
            msg_ptr += num_bytes;
            if (bytes_received >= 4 && 
                strncmp(msg_ptr - 4, "\r\n\r\n", 4) == 0)
                break;
        }
    }

    printf ("Received message from client:\n%s\n", msg_buffer);

}

int main (int argc, char **argv)
{
    if (argc != 2) 
    {
        printf ("Usage: %s <port>\n", argv[0]);
        printf ("ex) %s 12345\n", argv[0]);
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
