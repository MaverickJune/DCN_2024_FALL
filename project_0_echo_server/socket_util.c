// NXC Data Communications Network socket_util.c
// Written by Jongseok Park (cakeng@snu.ac.kr)
// 2023. 9. 7

#include "socket_util.h"

// Client side TCP socket initialization & connection function.
// Takes server_ip and server_port as arguments.
// Returns socket file descriptor if successful, -1 if not.
int client_init_and_connect_tcp_socket (char *server_ip, int server_port)
{
    int sock;
    struct sockaddr_in server_addr_info;

    // Create a socket that uses IP (PF_INET) and TCP (SOCK_STREAM)
    sock = socket(PF_INET, SOCK_STREAM, 0);
    if (sock == -1)
    {
        printf("socket() error\n");
        return -1;
    }

    // setsockopt() is used to set specific socket options.
    // In this case, we are setting the option of SO_REUSEADDR to val(1) to allow the reuse of local addresses, such as ports, for different connections.
    int val = 1;
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val));
    if (sock == -1)
    {
        printf("setsocketopt() SO_REUSEADDR error\n");
        return -1;
    }
    // Disable Nagle's algorithm
    // Nagle's algorithm is used to reduce the number of small packets sent over the network by buffering small packets and sending them together.
    // By disabling Nagle's algorithm, we can send small packets immediately.
    val = 1;
    setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, &val, sizeof(val));
    if (sock == -1)
    {
        printf("setsockopt() TCP_NODELAY error\n");
        return -1;
    }

    // Initialize server_addr_info to zero
    // memset() sets the sizeof(server_addr_info) bytes of memory, pointed to by &server_addr_info, to 0
    memset(&server_addr_info, 0, sizeof(server_addr_info));
    // Set server_addr_info to use IP for communication (AF_INET)
    server_addr_info.sin_family = AF_INET;
    // Convert server_ip address from string to binary and save it to server_addr_info using inet_addr()
    server_addr_info.sin_addr.s_addr = inet_addr(server_ip);
    // Convert server_port from host byte order to network byte order and save it to server_addr_info using htons()
    server_addr_info.sin_port = htons(server_port);

    // Try to connect to server with timeout of 60 seconds
    // If connection is successful, connect() returns 0
    // If connection is not successful, connect() returns -1
    int is_connected = connect(sock, (struct sockaddr *)&server_addr_info, sizeof(server_addr_info));
    float timeout = 60.0;
    int tries = 1;
    while (is_connected == -1)
    {
        sleep(1);
        timeout -= 1.0;
        if (timeout < 0.0)
        {
            printf("connect() timeout\n");
            return -1;
        }
        printf ("Connection to server %s:%d failed. Retrying... (%d tries)\r", server_ip, server_port, tries++);
        fflush(stdout);
        is_connected = connect(sock, (struct sockaddr *)&server_addr_info, sizeof(server_addr_info));
        if (is_connected == 0)
            printf ("\n");
    }

    printf ("Connected to server %s:%d\n", server_ip, server_port);

    return sock;
}

// Server side TCP socket initialization function.
// Takes server_port as argument.
// Returns socket file descriptor if successful, -1 if not.
int server_init_tcp_socket (int server_port)
{
    int sock;
    struct sockaddr_in server_addr_info;

    // Create a socket that uses IP (PF_INET) and TCP (SOCK_STREAM)
    sock = socket(PF_INET, SOCK_STREAM, 0);
    // setsockopt() is used to set specific socket options.
    // In this case, we are setting the option of SO_REUSEADDR to val(1) to allow the reuse of local addresses, such as ports, for different connections.
    int val = 1;
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val));
    if (sock == -1)
    {
        printf("setsocketopt() SO_REUSEADDR error\n");
        return -1;
    }
    // Disable Nagle's algorithm
    // Nagle's algorithm is used to reduce the number of small packets sent over the network by buffering small packets and sending them together.
    // By disabling Nagle's algorithm, we can send small packets immediately.
    val = 1;
    setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, &val, sizeof(val));
    if (sock == -1)
    {
        printf("setsockopt() TCP_NODELAY error\n");
        return -1;
    }

    // Initialize server_addr_info to zero
    // memset() sets the sizeof(server_addr_info) bytes of memory, pointed to by &server_addr_info, to 0
    memset(&server_addr_info, 0, sizeof(server_addr_info));
    // Set server_addr_info to use IP for communication (AF_INET)
    server_addr_info.sin_family = AF_INET;
    // Convert server_port from host byte order to network byte order and save it to server_addr_info using htons()
    server_addr_info.sin_port = htons(server_port);
    // Set server_addr_info to accept connections from any IP address
    server_addr_info.sin_addr.s_addr = htonl(INADDR_ANY);

    // Bind server_addr_info to socket (i.e., assign server_addr_info to our socket)
    if (bind(sock, (struct sockaddr *)&server_addr_info, sizeof(server_addr_info)) == -1)
    {
        printf("bind() error\n");
        return -1;
    }

    // The program will listen for any incoming connections on the socket.
    // The second argument is the maximum number of connections that can be queued before the system starts rejecting incoming connections.
    if (listen(sock, 5) == -1)
    {
        printf("listen() error\n");
        return -1;
    }

    printf ("Server listening on port %d\n", server_port);

    return sock;
}

// Accept incoming connections on the socket
int server_accept_tcp_socket (int sock)
{
    int client_sock;
    struct sockaddr_in client_addr_info;
    socklen_t client_addr_info_len = sizeof(client_addr_info);

    // Accept any incoming connections on the socket
    // If connection is successful, accept() returns a new socket file descriptor
    // If connection is not successful, accept() returns -1
    // If there are no pending connections, accept() blocks the execution of the program until a connection is established
    client_sock = accept(sock, (struct sockaddr *)&client_addr_info, &client_addr_info_len);
    if (client_sock == -1)
        printf("accept() error\n");

    // Print the IP address and port number of the client
    printf("Client %s:%d connected\n", inet_ntoa(client_addr_info.sin_addr), ntohs(client_addr_info.sin_port));

    return client_sock;
}
