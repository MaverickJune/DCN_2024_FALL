// NXC Data Communications torrent_engine.c for torrent
// Written by Jongseok Park (cakeng@snu.ac.kr)
// 2023. 9. 19


#include "torrent_engine.h"
#include "torrent_ui.h"
#include "torrent_utils.h"

// Client Routine.
int torrent_client (torrent_engine_t *engine)
{

    return 0;
}

// Server Routine.
int torrent_server (torrent_engine_t *engine)
{

    return 0;
}

// Open a socket and listen for incoming connections. 
// Returns the socket file descriptor on success, -1 on error.
int listen_socket (int port)
{
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) 
    {
        ERROR_PRTF ("ERROR listen_socket(): socket() failed.\n");
        return -1;
    }
    if (port < 0 || port >= 65536)
    {
        ERROR_PRTF ("ERROR listen_socket(): invalid port number.\n");
        return -1;
    }
    struct sockaddr_in serv_addr;
    memset (&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(port);

    int tr = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &tr, sizeof(int)) == -1)
    {
        ERROR_PRTF ("ERROR listen_socket(): setsockopt() failed.\n");
        close (sockfd);
        return -1;
    }
    if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
    {
        ERROR_PRTF ("ERROR listen_socket(): bind() failed.\n");
        close (sockfd);
        return -1;
    }
    if (listen(sockfd, MAX_QUEUED_CONNECTIONS) < 0)
    {
        ERROR_PRTF ("ERROR listen_socket(): listen() failed.\n");
        close (sockfd);
        return -1;
    }
    return sockfd;
}

// Accept an incoming connection with a timeout. 
// Returns the connected socket file descriptor on success, -1 on error, -2 on timeout.
// MUST have a timeout of TIMEOUT_MSEC msec.
// HINT: Use poll(). See kbhit() in torrent_util.c for an example on using poll().
int accept_socket(int listen_sock, struct sockaddr_in *cli_addr, socklen_t *clilen)
{
    struct pollfd fds;
    fds.fd = listen_sock;
    fds.events = POLLIN;
    int ret = poll(&fds, 1, TIMEOUT_MSEC);

    if (ret == 0) 
    {
        printf ("ERROR accept_socket(): poll() timeout.\n");
        return -2;
    }
    else if (ret < 0) 
    {
        ERROR_PRTF ("ERROR accept_socket(): poll() failed.\n");
        return -1;
    }

    int sockfd = accept(listen_sock, (struct sockaddr *) cli_addr, clilen);
    if (sockfd < 0) 
    {
        ERROR_PRTF ("ERROR accept_socket(): accept() failed.\n");
        return -1;
    }

    return sockfd;
}

// Connect to a server.
// Returns the socket file descriptor on success, -1 on error, -2 on timeout.
// MUST have a timeout of TIMEOUT_MSEC msec.
// HINT: Use poll(). See kbhit() in torrent_util.c for an example on using poll().
int connect_socket(char *server_ip, int port)
{
    int sockfd;
    struct sockaddr_in serv_addr;
    
    // Create a non-blocking socket.
    sockfd = socket (AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
    if (sockfd < 0) 
    {
        ERROR_PRTF ("ERROR connect_socket(): socket() failed.\n");
        return -1;
    }
    // Allow reuse of local addresses.
    int val = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val));
    if (sockfd == -1)
    {
        ERROR_PRTF ("ERROR connect_socket(): setsockopt() failed.\n");
        return -1;
    }
    // Disable Nagle's algorithm.
    val = 1;
    setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY, &val, sizeof(val));
    if (sockfd == -1)
    {
        ERROR_PRTF ("ERROR connect_socket(): setsockopt() failed.\n");
        return -1;
    }

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    serv_addr.sin_addr.s_addr = inet_addr(server_ip);

    connect(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr));

    // Poll for connection.
    struct pollfd fds;
    fds.fd = sockfd;
    fds.events = POLLIN;
    int ret = poll(&fds, 1, TIMEOUT_MSEC);

    if (ret == 0) 
    {
        printf ("ERROR connect_socket(): poll() timeout.\n");
        close (sockfd);
        return -2;
    }
    else if (ret < 0) 
    {
        ERROR_PRTF ("ERROR accept_socket(): poll() failed.\n");
        close (sockfd);
        return -1;
    }

    return sockfd;
}

// Request torrent info from a remote host. Returns 0 on success, -1 on error.
// Message protocol: "REQUEST_TORRENT_INFO [MY_LISTEN_PORT] [TORRENT_HASH]"
int request_torrent_info (peer_data_t *peer, torrent_t *torrent)
{
    if (peer == NULL || torrent == NULL)
    {
        ERROR_PRTF ("ERROR request_torrent_info(): invalid argument.\n");
        return -1;
    }

    // Format message.
    char msg[MSG_LEN] = {0};
    sprintf (msg, "REQUEST_TORRENT_INFO %d 0x%08x", torrent->engine->listen_sock, torrent->torrent_hash);
    
    // Connect to peer.
    int peer_sock = connect_socket (peer->ip, peer->port);
    if (peer_sock < 0)
    {
        ERROR_PRTF ("ERROR request_torrent_info(): connect_socket() failed.\n");
        return -1;
    }
    if (send (peer_sock, msg, strlen(msg), 0) < 0)
    {
        ERROR_PRTF ("ERROR request_torrent_info(): send() failed.\n");
        close (peer_sock);
        return -1;
    }
    close (peer_sock);

    return 0;
}

// Handle a request for torrent info, using push_torrent_info()
// Returns 0 on success, -1 on error.
int handle_request_torrent_info (torrent_engine_t *engine, char *msg)
{
    if (engine == NULL || msg == NULL)
    {
        ERROR_PRTF ("ERROR handle_request_torrent_info(): invalid argument.\n");
        return -1;
    }
    char *val = strtok (msg, " ");
    if (val == NULL || strncmp (val, "REQUEST_TORRENT_INFO", 20) != 0)
    {
        ERROR_PRTF ("ERROR handle_request_torrent_info(): invalid message.\n");
        return -1;
    }
    val = strtok (NULL, " ");

    return 0;
}

// Push torrent INFO to peer. Returns 0 on success, -1 on error.
// Message protocol: "PUSH_TORRENT_INFO [MY_LISTEN_PORT] [TORRENT_HASH] [TORRENT_NAME] [FILE_SIZE]"
int push_torrent_info (peer_data_t *peer, torrent_t *torrent)
{
    if (peer == NULL || torrent == NULL)
    {
        ERROR_PRTF ("ERROR push_torrent_info(): invalid argument.\n");
        return -1;
    }

    // Format message.
    char msg[MSG_LEN] = {0};
    sprintf (msg, "PUSH_TORRENT_INFO %d 0x%08x %s %ld", 
        torrent->engine->listen_sock, torrent->torrent_hash, torrent->torrent_name, torrent->file_size);

    // Connect to peer.
    int peer_sock = connect_socket (peer->ip, peer->port);
    if (peer_sock < 0)
    {
        ERROR_PRTF ("ERROR push_torrent_info(): connect_socket() failed.\n");
        return -1;
    }
    if (send (peer_sock, msg, strlen(msg), 0) < 0)
    {
        ERROR_PRTF ("ERROR push_torrent_info(): send() failed.\n");
        close (peer_sock);
        return -1;
    }
    close (peer_sock);

    return 0;
}

// Request peer's peer list from peer. Returns 0 on success, -1 on error.
int request_torrent_peers (peer_data_t *peer, torrent_t *torrent)
{
    return 0;
}

// Request peer's block info from peer. Returns 0 on success, -1 on error.
int request_torrent_block_status (peer_data_t *peer, torrent_t *torrent)
{
    return 0;
}

// Request block from peer. Returns 0 on success, -1 on error.
int request_torrent_block (peer_data_t *peer, torrent_t *torrent, size_t block_index)
{
    return 0;
}

// Push my list of peers to peer. Returns 0 on success, -1 on error.
// Be sure to remove the receiving peer from the peer list, if there is one.
int push_torrent_peers (peer_data_t *peer, torrent_t *torrent)
{
    return 0;
}

// Push torrent block status to peer. Returns 0 on success, -1 on error.
int push_torrent_block_status (peer_data_t *peer, torrent_t *torrent)
{
    return 0;
}

// Push block to peer. Returns 0 on success, -1 on error.
int push_torrent_block (peer_data_t *peer, torrent_t *torrent, size_t block_index)
{
    return 0;
}