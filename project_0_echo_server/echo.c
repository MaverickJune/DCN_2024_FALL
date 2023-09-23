// NXC Data Communications Network echo.c
// Written by Jongseok Park (cakeng@snu.ac.kr)
// 2023. 9. 7

#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "unistd.h"
#include "socket_util.h"

#define SERVER_MODE 1
#define CLIENT_MODE 2
#define MAX_ECHO_MSG_SIZE 1024 // Maximum size of message

// A server-side function that does something with the received string.
void server_function (char* str)
{
    // TODO
    // Reverse the string

    int len = strlen(str);
    char temp;
    for (int i = 0; i < len/2; i++)
    {
        temp = str[i];
        str[i] = str[len-i-1];
        str[len-i-1] = temp;
    }
}

// Server routine
int server_routine (int server_port)
{
    // Initialize server socket
    int server_listening_sock = server_init_tcp_socket(server_port);
    if (server_listening_sock == -1)
    {
        ERROR_PRTF ("ERROR server_routine(): server_init_tcp_socket() failed.\n");
        return -1;
    }

    // Wait for incoming connections
    int client_connected_sock = server_accept_tcp_socket(server_listening_sock);
    if (client_connected_sock == -1)
    {
        ERROR_PRTF ("ERROR server_routine(): server_accept_tcp_socket() failed.\n");
        return -1;
    }

    // Receive message from client, do something with it, and send it back to client, forever.
    char msg_buffer[MAX_ECHO_MSG_SIZE];
    ssize_t bytes_received = 0;
    while (1)
    {
        // Clear msg_buffer
        memset(msg_buffer, 0, MAX_ECHO_MSG_SIZE);

        // Receive message from client. 
        // read() returns -1 if error occurs, 0 if client disconnected, and number of bytes received if successful.
        bytes_received = read(client_connected_sock, msg_buffer, MAX_ECHO_MSG_SIZE);
        if (bytes_received == -1)
        {
            ERROR_PRTF ("ERROR server_routine(): read() failed.\n");
            return -1;
        }
        else if (bytes_received == 0)
        {
            ERROR_PRTF ("ERROR server_routine(): Client disconnected.\n");
            break; // Break out of while loop
        }
        msg_buffer[MAX_ECHO_MSG_SIZE - 1] = '\0'; // Make sure msg_buffer is null-terminated.
        printf ("Client message recieved: %s\n", msg_buffer);

        // Do something with the received string.
        server_function (msg_buffer);
        printf ("Responding with: %s\n", msg_buffer);
        fflush (stdout);
        
        // Send message back to client. 
        // write() returns -1 if error occurs, and number of bytes sent if successful.
        ssize_t bytes_sent = write(client_connected_sock, msg_buffer, MAX_ECHO_MSG_SIZE);
        if (bytes_sent == -1)
        {
            ERROR_PRTF ("ERROR server_routine(): write() failed.\n");
            return -1;
        }
    }

    // Cleanup
    printf ("Server exiting...\n");
    close(client_connected_sock);
    close(server_listening_sock);
    return 0;
}

// Client routine
int client_routine (char *server_ip, int server_port)
{
    // Initialize client socket & connect to server
    int client_sock = client_init_and_connect_tcp_socket (server_ip, server_port);
    if (client_sock == -1)
    {
        ERROR_PRTF ("ERROR client_routine(): client_init_and_connect_tcp_socket() failed.\n");
        return -1;
    }

    // Send message to server, receive message from server, and print it, forever.
    char msg_buffer[MAX_ECHO_MSG_SIZE];
    ssize_t bytes_received = 0;
    while (1)
    {
        // Clear msg_buffer
        memset(msg_buffer, 0, MAX_ECHO_MSG_SIZE);

        // Read message from stdin
        printf ("Enter your message to send to server. (\"exit\" to quit): ");
        fgets (msg_buffer, MAX_ECHO_MSG_SIZE, stdin);
        msg_buffer[strcspn(msg_buffer, "\n")] = 0; // Remove any trailing newline character from msg_buffer
        // Check if user entered "exit" to quit
        if (strncmp(msg_buffer, "exit", 4) == 0)
            break; // Break out of while loop

        // Send message to server
        // write() returns -1 if error occurs, and number of bytes sent if successful.
        ssize_t bytes_sent = write(client_sock, msg_buffer, MAX_ECHO_MSG_SIZE);
        if (bytes_sent == -1)
        {
            ERROR_PRTF ("ERROR client_routine(): write() failed.\n");
            return -1;
        }

        // Receive message from server
        // read() returns -1 if error occurs, 0 if server disconnected, and number of bytes received if successful.
        bytes_received = read(client_sock, msg_buffer, MAX_ECHO_MSG_SIZE);
        if (bytes_received == -1)
        {
            ERROR_PRTF ("ERROR client_routine(): read() failed.\n");
            return -1;
        }
        else if (bytes_received == 0)
        {
            ERROR_PRTF ("ERROR client_routine(): Server disconnected.\n");
            break; // Break out of while loop
        }
        msg_buffer[MAX_ECHO_MSG_SIZE - 1] = '\0'; // Make sure msg_buffer is null-terminated.
        
        // Print received message
        printf("Received message from server: %s\n", msg_buffer);
    }

    // Cleanup
    printf ("Client exiting...\n");
    close(client_sock);
    return 0;
}

// Main function
int main (int argc, char **argv)
{
    // Input argument check
    if (argc != 4)
    {
        printf("Usage:\t%s <program_mode> <server_ip> <server_port>\n", argv[0]);
        printf("\t\t<program_mode>: server or client\n");
        printf("\t\t<server_ip>: IP address of the server\n");
        printf("\t\t<server_port>: port number of the server\n");
        printf("\t\tex) %s server 127.0.0.1 12345\n", argv[0]);
        return 1;
    }
    
    char *program_mode_str = argv[1];
    char *server_ip = argv[2];
    int server_port = atoi(argv[3]);
    int program_mode = 0;
    if (strncmp(program_mode_str, "server", 7) == 0)
        program_mode = SERVER_MODE;
    else if (strncmp(program_mode_str, "client", 7) == 0)
        program_mode = CLIENT_MODE;
    else
    {
        ERROR_PRTF ("ERROR main(): Invalid program_mode_str.\n");
        return 1;
    }

    int ret = 0;
    // Run server or client routine
    if (program_mode == SERVER_MODE)
        ret = server_routine (server_port);
    else
        ret = client_routine (server_ip, server_port);

    if (ret != 0)
    {
        ERROR_PRTF ("ERROR main(): server_routine() or client_routine() failed.\n");
        return 1;
    }
    return 0;
}