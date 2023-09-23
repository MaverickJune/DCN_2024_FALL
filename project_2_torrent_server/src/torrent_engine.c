// NXC Data Communications torrent_engine.c for torrent
// Written by Jongseok Park (cakeng@snu.ac.kr)
// 2023. 9. 19


#include "torrent_engine.h"
#include "torrent_ui.h"
#include "torrent_utils.h"

int torrent_client (torrent_engine_t *engine)
{
    if (engine == NULL)
    {
        ERROR_PRTF ("ERROR torrent_client(): invalid argument.\n");
        return -1;
    }

    // Iterate through all torrents and take action.
    for (size_t i = 0; i < engine->num_torrents; i++)
    {
        torrent_t *torrent = engine->torrents[i];
        if (torrent == NULL)
        {
            ERROR_PRTF ("ERROR torrent_client(): invalid torrent.\n");
            return -1;
        }
        // If there are no peers, skip this torrent.
        if (torrent->num_peers == 0)
            continue;
        // Select a random peer.
        peer_data_t *peer = torrent->peers[rand() % torrent->num_peers];
        
        if (torrent->num_blocks == 0)
            request_torrent_info (peer, torrent);

        //// TODO: COMPLETE THE TORRENT CLIENT FUNCTION ////

    }

    return 0;
}

int torrent_server (torrent_engine_t *engine)
{
    if (engine == NULL)
    {
        ERROR_PRTF ("ERROR torrent_server(): invalid argument.\n");
        return -1;
    }

    // Accept incoming connections.
    struct sockaddr_in peer_addr_info;
    socklen_t peer_addr_info_len = sizeof(peer_addr_info);
    int peer_sock = accept_socket (engine->listen_sock, &peer_addr_info, &peer_addr_info_len);
    if (peer_sock < -1)
    {
        ERROR_PRTF ("ERROR torrent_server(): accept_socket() failed.\n");
        return -1;
    }
    else if (peer_sock == -1) // Timeout.
        return 0;

    
    char peer_ip[STR_LEN] = {0};
    strncpy (peer_ip, inet_ntoa(peer_addr_info.sin_addr), STR_LEN - 1);
    int peer_con_port = ntohs(peer_addr_info.sin_port);

    // Read message.
    char msg[MSG_LEN] = {0};
    if (read_bytes (peer_sock, msg, MSG_LEN) != MSG_LEN)
    {
        ERROR_PRTF ("ERROR torrent_server(): read_bytes() failed.\n");
        close (peer_sock);
        return -1;
    }

    INFO_PRTF ("\tINFO [%04.3fs] RECEIVED \"%s\" from peer %s:%d.\n", (double)get_elapsed_msec()/1000,
        msg, peer_ip, peer_con_port);

    char *cmd = strtok (msg, " ");
    if (cmd == NULL)
    {
        ERROR_PRTF ("ERROR torrent_server(): invalid message.\n");
        close (peer_sock);
        return -1;
    }
    char *peer_engine_hash = strtok (NULL, " ");
    if (peer_engine_hash == NULL || strlen(peer_engine_hash) != 10)
    {
        ERROR_PRTF ("ERROR torrent_server(): invalid message.\n");
        close (peer_sock);
        return -1;
    }
    else if (strtoul (peer_engine_hash, NULL, 16) == engine->engine_hash)
    {
        ERROR_PRTF ("ERROR torrent_server(): received message from self.\n");
        close (peer_sock);
        return -1;
    }
    char *msg_body = peer_engine_hash + 10;

    // Take action based on message.
    if (strncmp (msg, "REQUEST_TORRENT_INFO", 20) == 0)
        handle_request_torrent_info (engine, msg, peer_ip, peer_sock);
    else if (strncmp (msg, "PUSH_TORRENT_INFO", 17) == 0)
        handle_push_torrent_info (engine, msg, peer_ip, peer_sock);
    
    //// TODO: COMPLETE THE TORRENT SERVER FUNCTION ////

    else
    {
        ERROR_PRTF ("ERROR torrent_server(): invalid message.\n");
        return -1;
    }
    
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
// Returns the connected socket file descriptor on success, -2 on error, -1 on timeout.
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
        // ERROR_PRTF ("ERROR accept_socket(): poll() timeout.\n");
        return -1;
    }
    else if (ret < 0) 
    {
        ERROR_PRTF ("ERROR accept_socket(): poll() failed.\n");
        return -2;
    }

    int sockfd = accept(listen_sock, (struct sockaddr *) cli_addr, clilen);
    if (sockfd < 0) 
    {
        ERROR_PRTF ("ERROR accept_socket(): accept() failed.\n");
        return -2;
    }

    return sockfd;
}

// Connect to a server.
// Returns the socket file descriptor on success, -2 on error, -1 on timeout.
// MUST have a timeout of TIMEOUT_MSEC msec.
// HINT: Use poll(). See kbhit() in torrent_utils.c for an example on using poll().
int connect_socket(char *server_ip, int port)
{
    int sockfd;
    struct sockaddr_in serv_addr;
    
    // Create a non-blocking socket.
    sockfd = socket (AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
    if (sockfd < 0) 
    {
        ERROR_PRTF ("ERROR connect_socket(): socket() failed.\n");
        return -2;
    }
    // Allow reuse of local addresses.
    int val = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val));
    if (sockfd == -1)
    {
        ERROR_PRTF ("ERROR connect_socket(): setsockopt() failed.\n");
        return -2;
    }
    // Disable Nagle's algorithm.
    val = 1;
    setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY, &val, sizeof(val));
    if (sockfd == -1)
    {
        ERROR_PRTF ("ERROR connect_socket(): setsockopt() failed.\n");
        return -2;
    }

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    serv_addr.sin_addr.s_addr = inet_addr(server_ip);

    val = connect(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr));
    if (val == 0)
        return sockfd;

    // Poll for connection.
    struct pollfd fds;
    fds.fd = sockfd;
    fds.events = POLLOUT;
    int ret = poll(&fds, 1, TIMEOUT_MSEC);

    if (ret == 0) 
    {
        // ERROR_PRTF ("ERROR connect_socket(): poll() timeout.\n");
        close (sockfd);
        return -1;
    }
    else if (ret < 0) 
    {
        ERROR_PRTF ("ERROR connect_socket(): poll() failed.\n");
        close (sockfd);
        return -2;
    }

    // Check for errors.
    int err;
    socklen_t len = sizeof(err);
    if (getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &err, &len) < 0)
    {
        ERROR_PRTF ("ERROR connect_socket(): getsockopt() failed.\n");
        close (sockfd);
        return -2;
    }
    if (err != 0)
    {
        close (sockfd);
        return -1;
    }

    return sockfd;
}


// Request peer's peer list from peer. 
// Returns 0 on success, -1 on timeout, -2 on error.
int request_torrent_peer_list (peer_data_t *peer, torrent_t *torrent)
{
    return 0;
}

// Request peer's block info from peer. 
// Returns 0 on success, -1 on timeout, -2 on error.
int request_torrent_block_status (peer_data_t *peer, torrent_t *torrent)
{
    return 0;
}

// Request block from peer. 
// Returns 0 on success, -1 on timeout, -2 on error.
int request_torrent_block (peer_data_t *peer, torrent_t *torrent, size_t block_index)
{
    return 0;
}

// Push my list of peers to peer. 
// Returns 0 on success, -1 on timeout, -2 on error.
// Be sure to remove the receiving peer from the peer list, if there is one.
int push_torrent_peer_list (peer_data_t *peer, torrent_t *torrent)
{
    return 0;
}

// Push torrent block status to peer. 
// Returns 0 on success, -1 on timeout, -2 on error.
int push_torrent_block_status (peer_data_t *peer, torrent_t *torrent)
{
    return 0;
} 

// Push block to peer. 
// Returns 0 on success, -1 on timeout, -2 on error.
int push_torrent_block (peer_data_t *peer, torrent_t *torrent, size_t block_index)
{
    return 0;
}

// Handle a request for peer list, using push_torrent_peer_list()
// Returns 0 on success, -1 on error.
int handle_request_torrent_peer_list (torrent_engine_t *engine, char *msg, char *peer_ip, int peer_sock)
{
    return 0;
}

// Handle a request for block info, using push_torrent_block_status()
// Returns 0 on success, -1 on error.
int handle_request_torrent_block_status (torrent_engine_t *engine, char *msg, char *peer_ip, int peer_sock)
{
    return 0;
}

// Handle a request for block, using push_torrent_block()
// Returns 0 on success, -1 on error.
int handle_request_torrent_block (torrent_engine_t *engine, char *msg, char *peer_ip, int peer_sock)
{
    return 0;
}

// Handle a push of peer list.
// Returns 0 on success, -1 on error.
int handle_push_torrent_peer_list (torrent_engine_t *engine, char *msg, char *peer_ip, int peer_sock)
{
    return 0;
}

// Handle a push of block status.
// Returns 0 on success, -1 on error.
int handle_push_torrent_block_status (torrent_engine_t *engine, char *msg, char *peer_ip, int peer_sock)
{
    return 0;
}

// Handle a push of block.
// Returns 0 on success, -1 on error.
int handle_push_torrent_block (torrent_engine_t *engine, char *msg, char *peer_ip, int peer_sock)
{
    return 0;
}

// Request torrent info from a remote host. Returns 0 on success, -1 on timeout, -2 on error.
// Message protocol: "REQUEST_TORRENT_INFO [MY_ENGINE_HASH] [MY_LISTEN_PORT] [TORRENT_HASH]"
int request_torrent_info (peer_data_t *peer, torrent_t *torrent)
{
    if (peer == NULL || torrent == NULL)
    {
        ERROR_PRTF ("ERROR request_torrent_info(): invalid argument.\n");
        return -2;
    }

    // Format message.
    char msg[MSG_LEN] = {0};
    sprintf (msg, "REQUEST_TORRENT_INFO 0x%08x %d 0x%08x", torrent->engine->engine_hash,
        torrent->engine->port, torrent->torrent_hash);
    
    // Connect to peer.
    int peer_sock = connect_socket (peer->ip, peer->port);
    if (peer_sock == -1) // Timeout.
    {
        INFO_PRTF ("\tINFO [%04.3fs] REQUESTING 0x%08x INFO FROM %s:%d - TIMEOUT.\n", (double)get_elapsed_msec()/1000, 
            torrent->torrent_hash, peer->ip, peer->port);
        return -1;
    }
    else if (peer_sock < -1)
    {
        ERROR_PRTF ("ERROR request_torrent_info(): connect_socket() failed.\n");
        return -2;
    }
    INFO_PRTF ("\tINFO [%04.3fs] REQUESTING 0x%08x INFO FROM %s:%d.\n", (double)get_elapsed_msec()/1000, 
        torrent->torrent_hash, peer->ip, peer->port);
    // Send message.
    if (write_bytes (peer_sock, msg, MSG_LEN) != MSG_LEN)
    {
        ERROR_PRTF ("ERROR request_torrent_info(): send() failed - %d\n", peer_sock);
        close (peer_sock);
        return -2;
    }
    close (peer_sock);

    return 0;
}

// Push torrent INFO to peer. Returns 0 on success, -1 on timeout, -2 on error.
// Message protocol: 
// "PUSH_TORRENT_INFO [MY_ENGINE_HASH] [MY_LISTEN_PORT] [TORRENT_HASH] [TORRENT_NAME] [FILE_SIZE]" [BLOCK_HASH]
// [BLOCK_HASH] is a binary dump of the torrent's block hashes.
int push_torrent_info (peer_data_t *peer, torrent_t *torrent)
{
    if (peer == NULL || torrent == NULL)
    {
        ERROR_PRTF ("ERROR push_torrent_info(): invalid argument.\n");
        return -2;
    }

    // Format message.
    char msg[MSG_LEN] = {0};
    sprintf (msg, "PUSH_TORRENT_INFO 0x%08x %d 0x%08x %s %ld", torrent->engine->engine_hash,
        torrent->engine->port, torrent->torrent_hash, torrent->torrent_name, torrent->file_size);

    // Connect to peer.
    int peer_sock = connect_socket (peer->ip, peer->port);
    if (peer_sock == -1) // Timeout.
    {
        INFO_PRTF ("\tINFO [%04.3fs] PUSHING 0x%08x INFO TO %s:%d - TIMEOUT.\n", (double)get_elapsed_msec()/1000, 
            torrent->torrent_hash, peer->ip, peer->port);
        return -1;
    }
    else if (peer_sock < -1)
    {
        ERROR_PRTF ("ERROR push_torrent_info(): connect_socket() failed.\n");
        return -2;
    }

    INFO_PRTF ("\tINFO [%04.3fs] PUSHING 0x%08x INFO TO %s:%d.\n", (double)get_elapsed_msec()/1000, 
        torrent->torrent_hash, peer->ip, peer->port);

    // Send message.
    if (write_bytes (peer_sock, msg, MSG_LEN) != MSG_LEN)
    {
        ERROR_PRTF ("ERROR push_torrent_info(): send() failed.\n");
        close (peer_sock);
        return -2;
    }

    // Send block hashes.
    if (write_bytes (peer_sock, (char*) torrent->block_hashes, torrent->num_blocks * sizeof(HASH_t)) != 
        torrent->num_blocks * sizeof(HASH_t))
    {
        ERROR_PRTF ("ERROR push_torrent_info(): send() failed.\n");
        close (peer_sock);
        return -2;
    }
    close (peer_sock);

    return 0;
}

// Handle a request for torrent info, using push_torrent_info()
// Returns 0 on success, -1 on error.
int handle_request_torrent_info (torrent_engine_t *engine, char *msg, char *peer_ip, int peer_sock)
{
    if (peer_sock < 0)
    {
        ERROR_PRTF ("ERROR handle_request_torrent_info(): invalid socket.\n");
        return -1;
    }
    if (engine == NULL || msg == NULL || peer_ip == NULL)
    {
        ERROR_PRTF ("ERROR handle_request_torrent_info(): invalid argument.\n");
        close (peer_sock);
        return -1;
    }
    close (peer_sock);

    // Parse the listen port number of the peer.
    char *val = strtok (NULL, " ");
    int peer_port = atoi (val);
    if (peer_port < 0 || peer_port >= 65536)
    {
        ERROR_PRTF ("ERROR handle_request_torrent_info(): invalid port number.\n");
        return -1;
    }

    // Parse torrent hash.
    val = strtok (NULL, " ");
    HASH_t torrent_hash = strtoul (val, NULL, 16);

    // Check for extra arguments.
    val = strtok (NULL, " ");
    if (val != NULL)
    {
        ERROR_PRTF ("ERROR handle_request_torrent_info(): invalid message.\n");
        return -1;
    }

    INFO_PRTF ("\tINFO [%04.3fs] RECEIVED REQUEST FOR 0x%08x INFO FROM %s:%d\n", (double)get_elapsed_msec()/1000,
        torrent_hash, peer_ip, peer_port);

    // Find torrent using torrent hash.
    ssize_t torrent_index = find_torrent (engine, torrent_hash);
    if (torrent_index == -1)
    {
        // ERROR_PRTF ("ERROR handle_request_torrent_info(): torrent not found.\n");
        return -1;
    }
    torrent_t *torrent = engine->torrents[torrent_index];

    // Add peer to torrent.
    ssize_t peer_index = find_peer (torrent, peer_ip, peer_port);
    if (peer_index == -1)
    {
        peer_index = torrent_add_peer (torrent, peer_ip, peer_port);
        if (peer_index == -1)
        {
            ERROR_PRTF ("ERROR handle_request_torrent_info(): torrent_add_peer() failed.\n");
            return -1;
        }
    }
    peer_data_t *peer = torrent->peers[peer_index];

    // Push torrent info to peer.
    if (push_torrent_info (peer, torrent) < -1)
    {
        ERROR_PRTF ("ERROR handle_request_torrent_info(): push_torrent_info() failed.\n");
        return -1;
    }

    return 0;
}


// Handle a push of torrent info.
// Returns 0 on success, -1 on error.
int handle_push_torrent_info (torrent_engine_t *engine, char *msg, char *peer_ip, int peer_sock)
{
    if (peer_sock < 0)
    {
        ERROR_PRTF ("ERROR handle_push_torrent_info(): invalid socket.\n");
        return -1;
    }
    if (engine == NULL || msg == NULL || peer_ip == NULL)
    {
        ERROR_PRTF ("ERROR handle_push_torrent_info(): invalid argument.\n");
        close (peer_sock);
        return -1;
    }

    // Parse the listen port number of the peer.
    char *val = strtok (NULL, " ");
    int peer_port = atoi (val);
    if (peer_port < 0 || peer_port >= 65536)
    {
        ERROR_PRTF ("ERROR handle_push_torrent_info(): invalid port number.\n");
        close (peer_sock);
        return -1;
    }

    // Parse torrent hash.
    val = strtok (NULL, " ");
    HASH_t torrent_hash = strtoul (val, NULL, 16);

    // Parse torrent name.
    val = strtok (NULL, " ");
    char torrent_name[STR_LEN] = {0};
    strncpy (torrent_name, val, STR_LEN - 1);

    // Parse file size.
    val = strtok (NULL, " ");
    size_t file_size = atoi (val);

    // Check for extra arguments.
    val = strtok (NULL, " ");
    if (val != NULL)
    {
        ERROR_PRTF ("ERROR handle_push_torrent_info(): invalid message.\n");
        close (peer_sock);
        return -1;
    }

    INFO_PRTF ("\tINFO [%04.3fs] RECEIVED PUSH FOR 0x%08x INFO FROM %s:%d- NAME: %s, SIZE: %ld\n"
        , (double)get_elapsed_msec()/1000, torrent_hash, peer_ip, peer_port, torrent_name, file_size);
    
    // Find torrent using torrent hash.
    ssize_t torrent_index = find_torrent (engine, torrent_hash);
    if (torrent_index == -1)
    {
        // ERROR_PRTF ("ERROR handle_push_torrent_info(): torrent not found.\n");
        close (peer_sock);
        return -1;
    }
    torrent_t *torrent = engine->torrents[torrent_index];

    // Find peer using peer ip and port.
    ssize_t peer_index = find_peer (torrent, peer_ip, peer_port);
    if (peer_index == -1)
    {
        peer_index = torrent_add_peer (torrent, peer_ip, peer_port);
        if (peer_index == -1)
        {
            ERROR_PRTF ("ERROR handle_push_torrent_info(): torrent_add_peer() failed.\n");
            close (peer_sock);
            return -1;
        }
    }
    peer_data_t *peer = torrent->peers[peer_index];

    // Check if torrent already has info.
    if (torrent->num_blocks > 0)
    {
        // ERROR_PRTF ("ERROR handle_push_torrent_info(): torrent already has info.\n");
        close (peer_sock);
        return -1;
    }

    // Update torrent info.
    if (set_torrent_info (torrent, torrent_name, file_size) < 0)
    {
        ERROR_PRTF ("ERROR handle_push_torrent_info(): set_torrent_info() failed.\n");
        close (peer_sock);
        return -1;
    }

    // Read block hashes.
    if (read_bytes (peer_sock, (char*) torrent->block_hashes, torrent->num_blocks * sizeof(HASH_t)) != 
        torrent->num_blocks * sizeof(HASH_t))
    {
        ERROR_PRTF ("ERROR handle_push_torrent_info(): read_bytes() failed.\n");
        close (peer_sock);
        return -1;
    }
    close (peer_sock);

    return 0;
}