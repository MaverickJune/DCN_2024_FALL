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
        // Select a random peer using rand().
        peer_data_t *peer = torrent->peers[rand() % torrent->num_peers];

        // If peer has more num_requests than PEER_EVICT_REQUEST_NUM, evict the peer.
        if (peer->num_requests > PEER_EVICT_REQUEST_NUM)
        {
            INFO_PRTF ("\t[%04.3fs] EVICTING 0x%08x PEER %s:%d.\n", (double)get_elapsed_msec()/1000, 
                torrent->torrent_hash, peer->ip, peer->port);
            if (torrent_remove_peer (torrent, peer->ip, peer->port) < 0)
            {
                ERROR_PRTF ("ERROR torrent_client(): torrent_remove_peer() failed.\n");
                return -1;
            }
            continue;
        }

        // If RESET_BLOCK_STATUS_INTERVAL_MSEC has passed since last reset, reset block status.
        if (get_elapsed_msec() - peer->last_block_status_reset_msec > RESET_BLOCK_STATUS_INTERVAL_MSEC)
        {
            INFO_PRTF ("\t[%04.3fs] RESETING 0x%08x BLOCK STATUS.\n", (double)get_elapsed_msec()/1000,
                 torrent->torrent_hash);
            for (size_t j = 0; j < torrent->num_blocks; j++)
                if (get_block_status (torrent, j) == B_REQUESTED)
                    torrent->block_status[j] = B_MISSING;
            peer->last_block_status_reset_msec = get_elapsed_msec();
        }
        
        // If torrent info has not been received, request the info from the peer.
        if (torrent->num_blocks == 0)
        {
            if (get_elapsed_msec() - peer->last_torrent_info_request_msec > REQUEST_TORRENT_INFO_INTERVAL_MSEC)
                if (request_torrent_info (peer, torrent) < -1)
                {
                    ERROR_PRTF ("ERROR torrent_client(): request_torrent_info() failed.\n");
                    return -1;
                }
        }
        else
        {
            // If REQUEST_PEER_LIST_INTERVAL_MSEC has passed since last request, request peer list.
            if (get_elapsed_msec() - peer->last_peer_list_request_msec > REQUEST_PEER_LIST_INTERVAL_MSEC)
            {
                if (request_torrent_peer_list (peer, torrent) < -1)
                {
                    ERROR_PRTF ("ERROR torrent_client(): request_torrent_peer_list() failed.\n");
                    return -1;
                }
            }
            // If REQUEST_BLOCK_STATUS_INTERVAL_MSEC has passed since last request, request block status.
            else if (get_elapsed_msec() - peer->last_block_status_request_msec > REQUEST_BLOCK_STATUS_INTERVAL_MSEC)
            {
                if (request_torrent_block_status (peer, torrent) < -1)
                {
                    ERROR_PRTF ("ERROR torrent_client(): request_torrent_block_status() failed.\n");
                    return -1;
                }
            }
            // If REQUEST_BLOCK_INTERVAL_MSEC has passed since last request, request a block.
            else if (get_elapsed_msec() - peer->last_block_request_msec > REQUEST_BLOCK_INTERVAL_MSEC)
            {            
                ssize_t block_index = get_missing_block (torrent, 0);
                // If there are no missing blocks, save the torrent as a file.
                if (block_index == -1)
                {
                    if (save_torrent_as_file (torrent) < 0)
                    {
                        ERROR_PRTF ("ERROR torrent_client(): save_torrent_as_file() failed.\n");
                        return -1;
                    }
                }
                else if (block_index >= 0)
                {
                    // Request the first missing block which the peer has.
                    while (block_index >= 0)
                    {
                        // Peer has the missing block.
                        if (get_peer_block_status (peer, block_index) == B_COMPLETED)
                        {
                            if (request_torrent_block (peer, torrent, block_index) < -1)
                            {
                                ERROR_PRTF ("ERROR torrent_client(): request_torrent_block() failed.\n");
                                return -1;
                            }
                        }
                        // Peer does not any of the missing blocks.
                        if (block_index + 1 >= torrent->num_blocks)
                            break;
                        // Get the next missing block.
                        block_index = get_missing_block (torrent, block_index + 1);
                    }
                }
                else if (block_index < -1)
                {
                    ERROR_PRTF ("ERROR torrent_client(): get_missing_block() failed.\n");
                    return -1;
                }
            }
        }
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
    // If there is a connection, handle it.
    // Differentiate between errors and timeouts.
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

    // Get peer ip and port.
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

    INFO_PRTF ("\t[%04.3fs] RECEIVED \"%s\" from peer %s:%d.\n", (double)get_elapsed_msec()/1000,
        msg, peer_ip, peer_con_port);

    // Parse command from message.
    char *cmd = strtok (msg, " ");
    if (cmd == NULL)
    {
        ERROR_PRTF ("ERROR torrent_server(): invalid message.\n");
        close (peer_sock);
        return -1;
    }
    
    // Parse peer engine hash from message.
    // If the peer engine hash is the same as the local engine hash, ignore the message.
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

    // Parse peer listen port from message.
    char *peer_listen_port_str = strtok (NULL, " ");
    if (peer_listen_port_str == NULL)
    {
        ERROR_PRTF ("ERROR torrent_server(): invalid message.\n");
        close (peer_sock);
        return -1;
    }
    int peer_listen_port = atoi (peer_listen_port_str);
    if (peer_listen_port < 0 || peer_listen_port >= 65536)
    {
        ERROR_PRTF ("ERROR torrent_server(): invalid port number.\n");
        close (peer_sock);
        return -1;
    }

    // Parse peer torrent hash from message.
    char *peer_torrent_hash_str = strtok (NULL, " ");
    if (peer_torrent_hash_str == NULL || strlen(peer_torrent_hash_str) != 10)
    {
        ERROR_PRTF ("ERROR torrent_server(): invalid message.\n");
        close (peer_sock);
        return -1;
    }
    HASH_t peer_torrent_hash = strtoul (peer_torrent_hash_str, NULL, 16);
    ssize_t torrent_index = find_torrent (engine, peer_torrent_hash);
    if (torrent_index < 0)
    {
        // ERROR_PRTF ("ERROR torrent_server(): invalid torrent hash.\n");
        close (peer_sock);
        return -1;
    }
    torrent_t *torrent = engine->torrents[torrent_index];

    // Add peer to torrent if it doesn't exist.
    ssize_t peer_idx = find_peer (torrent, peer_ip, peer_listen_port);
    if (peer_idx < 0)
    {
        peer_idx = torrent_add_peer (torrent, peer_ip, peer_listen_port);
        if (peer_idx < 0)
        {
            ERROR_PRTF ("ERROR torrent_server(): torrent_add_peer() failed.\n");
            close (peer_sock);
            return -1;
        }
    }
    peer_data_t *peer = torrent->peers[peer_idx];

    char *msg_body = strtok (NULL, "\0");

    // Take action based on message.
    if (strncmp (cmd, "REQUEST_TORRENT_INFO", 20) == 0)
        handle_request_torrent_info (engine, peer_sock, peer, torrent, msg_body);
    else if (strncmp (cmd, "PUSH_TORRENT_INFO", 17) == 0)
        handle_push_torrent_info (engine, peer_sock, peer, torrent, msg_body);
    else if (strncmp (cmd, "REQUEST_TORRENT_PEER_LIST", 25) == 0)
        handle_request_torrent_peer_list (engine, peer_sock, peer, torrent, msg_body);
    else if (strncmp (cmd, "PUSH_TORRENT_PEER_LIST", 22) == 0)
        handle_push_torrent_peer_list (engine, peer_sock, peer, torrent, msg_body);
    else if (strncmp (cmd, "REQUEST_TORRENT_BLOCK_STATUS", 28) == 0)
        handle_request_torrent_block_status (engine, peer_sock, peer, torrent, msg_body);
    else if (strncmp (cmd, "PUSH_TORRENT_BLOCK_STATUS", 25) == 0)
        handle_push_torrent_block_status (engine, peer_sock, peer, torrent, msg_body);
    else if (strncmp (cmd, "REQUEST_TORRENT_BLOCK", 21) == 0)
        handle_request_torrent_block (engine, peer_sock, peer, torrent, msg_body);
    else if (strncmp (cmd, "PUSH_TORRENT_BLOCK", 18) == 0)
        handle_push_torrent_block (engine, peer_sock, peer, torrent, msg_body);
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
// Message protocol: REQUEST_TORRENT_PEER_LIST [MY_ENGINE_HASH] [MY_LISTEN_PORT] [TORRENT_HASH]
int request_torrent_peer_list (peer_data_t *peer, torrent_t *torrent)
{
    if (peer == NULL || torrent == NULL)
    {
        ERROR_PRTF ("ERROR request_torrent_peer_list(): invalid argument.\n");
        return -2;
    }

    // Format message.
    char msg[MSG_LEN] = {0};
    sprintf (msg, "REQUEST_TORRENT_PEER_LIST 0x%08x %d 0x%08x", torrent->engine->engine_hash,
        torrent->engine->port, torrent->torrent_hash);

    // Connect to peer.
    int peer_sock = connect_socket (peer->ip, peer->port);
    if (peer_sock == -1)
    {
        INFO_PRTF ("\t[%04.3fs] REQUESTING 0x%08x PEER LIST FROM %s:%d - TIMEOUT.\n", (double)get_elapsed_msec()/1000, 
            torrent->torrent_hash, peer->ip, peer->port);
        return -1;
    }
    else if (peer_sock < -1)
    {
        ERROR_PRTF ("ERROR request_torrent_peer_list(): connect_socket() failed.\n");
        return -2;
    }

    INFO_PRTF ("\t[%04.3fs] REQUESTING 0x%08x PEER LIST FROM %s:%d.\n", (double)get_elapsed_msec()/1000, 
        torrent->torrent_hash, peer->ip, peer->port);

    // Send message.
    if (write_bytes (peer_sock, msg, MSG_LEN) != MSG_LEN)
    {
        ERROR_PRTF ("ERROR request_torrent_peer_list(): send() failed - %d\n", peer_sock);
        close (peer_sock);
        return -2;
    }
    close (peer_sock);

    // Update peer info.
    peer->num_requests++;
    peer->last_peer_list_request_msec = get_elapsed_msec();

    return 0;
}

// Request peer's block info from peer. 
// Returns 0 on success, -1 on timeout, -2 on error.
// Message protocol: REQUEST_TORRENT_BLOCK_STATUS [MY_ENGINE_HASH] [MY_LISTEN_PORT] [TORRENT_HASH]
int request_torrent_block_status (peer_data_t *peer, torrent_t *torrent)
{
    if (peer == NULL || torrent == NULL)
    {
        ERROR_PRTF ("ERROR request_torrent_block_status(): invalid argument.\n");
        return -2;
    }

    // Format message.
    char msg[MSG_LEN] = {0};
    sprintf (msg, "REQUEST_TORRENT_BLOCK_STATUS 0x%08x %d 0x%08x", torrent->engine->engine_hash,
        torrent->engine->port, torrent->torrent_hash);

    // Connect to peer.
    int peer_sock = connect_socket (peer->ip, peer->port);
    if (peer_sock == -1)
    {
        INFO_PRTF ("\t[%04.3fs] REQUESTING 0x%08x BLOCK STATUS FROM %s:%d - TIMEOUT.\n", (double)get_elapsed_msec()/1000, 
            torrent->torrent_hash, peer->ip, peer->port);
        return -1;
    }
    else if (peer_sock < -1)
    {
        ERROR_PRTF ("ERROR request_torrent_block_status(): connect_socket() failed.\n");
        return -2;
    }

    INFO_PRTF ("\t[%04.3fs] REQUESTING 0x%08x BLOCK STATUS FROM %s:%d.\n", (double)get_elapsed_msec()/1000, 
        torrent->torrent_hash, peer->ip, peer->port);

    // Send message.
    if (write_bytes (peer_sock, msg, MSG_LEN) != MSG_LEN)
    {
        ERROR_PRTF ("ERROR request_torrent_block_status(): send() failed - %d\n", peer_sock);
        close (peer_sock);
        return -2;
    }
    close (peer_sock);

    // Update peer info.
    peer->num_requests++;
    peer->last_block_status_request_msec = get_elapsed_msec();

    return 0;
}

// Request block from peer. 
// Returns 0 on success, -1 on timeout, -2 on error.
// Message protocol: REQUEST_TORRENT_BLOCK [MY_ENGINE_HASH] [MY_LISTEN_PORT] [TORRENT_HASH] [BLOCK_INDEX]
int request_torrent_block (peer_data_t *peer, torrent_t *torrent, size_t block_index)
{
    if (peer == NULL || torrent == NULL)
    {
        ERROR_PRTF ("ERROR request_torrent_block(): invalid argument.\n");
        return -2;
    }

    // Update peer info.
    peer->num_requests++;
    peer->last_block_request_msec = get_elapsed_msec();

    // Format message.
    char msg[MSG_LEN] = {0};
    sprintf (msg, "REQUEST_TORRENT_BLOCK 0x%08x %d 0x%08x %ld", torrent->engine->engine_hash,
        torrent->engine->port, torrent->torrent_hash, block_index);

    // Connect to peer.
    int peer_sock = connect_socket (peer->ip, peer->port);
    if (peer_sock == -1)
    {
        INFO_PRTF ("\t[%04.3fs] REQUESTING 0x%08x BLOCK %ld FROM %s:%d - TIMEOUT.\n", (double)get_elapsed_msec()/1000, 
            torrent->torrent_hash, block_index, peer->ip, peer->port);
        return -1;
    }
    else if (peer_sock < -1)
    {
        ERROR_PRTF ("ERROR request_torrent_block(): connect_socket() failed.\n");
        return -2;
    }

    INFO_PRTF ("\t[%04.3fs] REQUESTING 0x%08x BLOCK %ld FROM %s:%d.\n", (double)get_elapsed_msec()/1000, 
        torrent->torrent_hash, block_index, peer->ip, peer->port);
    
    // Send message.
    if (write_bytes (peer_sock, msg, MSG_LEN) != MSG_LEN)
    {
        ERROR_PRTF ("ERROR request_torrent_block(): send() failed - %d\n", peer_sock);
        close (peer_sock);
        return -2;
    }
    close (peer_sock);

    // Set block status to requested.
    if (get_block_status (torrent, block_index) == B_MISSING)
        torrent->block_status[block_index] = B_REQUESTED;

    return 0;
}

// Push my list of peers to peer. 
// Returns 0 on success, -1 on timeout, -2 on error.
// Message protocol: 
// PUSH_TORRENT_PEER_LIST [MY_ENGINE_HASH] [MY_LISTEN_PORT] [TORRENT_HASH] [NUM_PEERS] [PEER_0_IP]:[PEER_0_PORT] [PEE_1_IP]:[PEER_1_PORT] ...
// [PEER_X_IP]:[PEER_X_PORT] is a list of peers, which starts AFTER MSG_LEN bytes in the message.
// Each the list of peers is  [NUM_PEERS] * PEER_LIST_BYTE_PER_PEER bytes long, including the space.
// Make sure not to send the IP and port of the receiving peer back to itself.
int push_torrent_peer_list (peer_data_t *peer, torrent_t *torrent)
{
    if (peer == NULL || torrent == NULL)
    {
        ERROR_PRTF ("ERROR push_torrent_peer_list(): invalid argument.\n");
        return -2;
    }

    // Check if requesting peer is in peer list.
    int peer_in_list = 0;
    if (find_peer (torrent, peer->ip, peer->port) >= 0)
        peer_in_list = 1;

    // Format message.
    char msg[MSG_LEN] = {0};
    sprintf (msg, "PUSH_TORRENT_PEER_LIST 0x%08x %d 0x%08x %ld", torrent->engine->engine_hash,
        torrent->engine->port, torrent->torrent_hash, torrent->num_peers - peer_in_list);
    
    // Connect to peer.
    int peer_sock = connect_socket (peer->ip, peer->port);
    if  (peer_sock == -1)
    {
        INFO_PRTF ("\t[%04.3fs] PUSHING 0x%08x PEER LIST TO %s:%d - TIMEOUT.\n", (double)get_elapsed_msec()/1000, 
            torrent->torrent_hash, peer->ip, peer->port);
        return -1;
    }
    else if (peer_sock < -1)
    {
        ERROR_PRTF ("ERROR push_torrent_peer_list(): connect_socket() failed.\n");
        return -2;
    }

    INFO_PRTF ("\t[%04.3fs] PUSHING 0x%08x PEER LIST TO %s:%d.\n", (double)get_elapsed_msec()/1000, 
        torrent->torrent_hash, peer->ip, peer->port);
    
    // Send message.
    if (write_bytes (peer_sock, msg, MSG_LEN) != MSG_LEN)
    {
        ERROR_PRTF ("ERROR push_torrent_peer_list(): send() failed - %d\n", peer_sock);
        close (peer_sock);
        return -2;
    }

    // Send peer list.
    size_t buffer_size = (torrent->num_peers - peer_in_list) * PEER_LIST_MAX_BYTE_PER_PEER + 1;
    char *buffer = calloc (1, buffer_size);
    if (buffer == NULL)
    {
        ERROR_PRTF ("ERROR push_torrent_peer_list(): calloc() failed.\n");
        close (peer_sock);
        return -2;
    }
    for (size_t i = 0; i < torrent->num_peers; i++)
    {
        if (strcmp (torrent->peers[i]->ip, peer->ip) == 0 && torrent->peers[i]->port == peer->port)
            continue;
        sprintf (buffer + strlen(buffer), "%s:%d ", torrent->peers[i]->ip, torrent->peers[i]->port);
    }
    buffer [buffer_size - 1] = '\0'; // Remove last space.
    if (write_bytes (peer_sock, buffer, buffer_size) != buffer_size)
    {
        ERROR_PRTF ("ERROR push_torrent_peer_list(): send() failed - %d\n", peer_sock);
        free (buffer);
        close (peer_sock);
        return -2;
    }
    free (buffer);
    close (peer_sock);

    return 0;
}

// Push torrent block status to peer. 
// Returns 0 on success, -1 on timeout, -2 on error.
// Message protocol: 
// PUSH_TORRENT_BLOCK_STATUS [MY_ENGINE_HASH] [MY_LISTEN_PORT] [TORRENT_HASH] [BLOCK_STATUS]
// [BLOCK_STATUS] is a binary dump of the torrent's block status, which starts AFTER MSG_LEN bytes in the message.
int push_torrent_block_status (peer_data_t *peer, torrent_t *torrent)
{
    if (peer == NULL || torrent == NULL)
    {
        ERROR_PRTF ("ERROR push_torrent_block_status(): invalid argument.\n");
        return -2;
    }

    // Format message.
    char msg[MSG_LEN] = {0};
    sprintf (msg, "PUSH_TORRENT_BLOCK_STATUS 0x%08x %d 0x%08x", torrent->engine->engine_hash,
        torrent->engine->port, torrent->torrent_hash);

    // Connect to peer.
    int peer_sock = connect_socket (peer->ip, peer->port);
    if (peer_sock == -1)
    {
        INFO_PRTF ("\t[%04.3fs] PUSHING 0x%08x BLOCK STATUS TO %s:%d - TIMEOUT.\n", (double)get_elapsed_msec()/1000, 
            torrent->torrent_hash, peer->ip, peer->port);
        return -1;
    }
    else if (peer_sock < -1)
    {
        ERROR_PRTF ("ERROR push_torrent_block_status(): connect_socket() failed.\n");
        return -2;
    }

    INFO_PRTF ("\t[%04.3fs] PUSHING 0x%08x BLOCK STATUS TO %s:%d.\n", (double)get_elapsed_msec()/1000, 
        torrent->torrent_hash, peer->ip, peer->port);

    // Send message.
    if (write_bytes (peer_sock, msg, MSG_LEN) != MSG_LEN)
    {
        ERROR_PRTF ("ERROR push_torrent_block_status(): send() failed - %d\n", peer_sock);
        close (peer_sock);
        return -2;
    }

    // Send block status.
    if (write_bytes (peer_sock, torrent->block_status, torrent->num_blocks * sizeof (B_STAT)) != torrent->num_blocks * sizeof (B_STAT))
    {
        ERROR_PRTF ("ERROR push_torrent_block_status(): send() failed - %d\n", peer_sock);
        close (peer_sock);
        return -2;
    }

    close (peer_sock);

    return 0;
} 

// Push block to peer. 
// Returns 0 on success, -1 on timeout, -2 on error.
// Message protocol:
// PUSH_TORRENT_BLOCK [MY_ENGINE_HASH] [MY_LISTEN_PORT] [TORRENT_HASH] [BLOCK_INDEX] [BLOCK_DATA]
// [BLOCK_DATA] is a binary dump of the block data, which starts AFTER MSG_LEN bytes in the message.
int push_torrent_block (peer_data_t *peer, torrent_t *torrent, size_t block_index)
{
    if (peer == NULL || torrent == NULL)
    {
        ERROR_PRTF ("ERROR push_torrent_block(): invalid argument.\n");
        return -2;
    }
    if (block_index >= torrent->num_blocks || torrent->block_status[block_index] != 1)
    {
        ERROR_PRTF ("ERROR push_torrent_block(): invalid block index.\n");
        return -2;
    }

    // Format message.
    char msg[MSG_LEN] = {0};
    sprintf (msg, "PUSH_TORRENT_BLOCK 0x%08x %d 0x%08x %ld", torrent->engine->engine_hash,
        torrent->engine->port, torrent->torrent_hash, block_index);
    
    // Connect to peer.
    int peer_sock = connect_socket (peer->ip, peer->port);
    if (peer_sock == -1)
    {
        INFO_PRTF ("\t[%04.3fs] PUSHING 0x%08x BLOCK %ld TO %s:%d - TIMEOUT.\n", (double)get_elapsed_msec()/1000, 
            torrent->torrent_hash, block_index, peer->ip, peer->port);
        return -1;
    }
    else if (peer_sock < -1)
    {
        ERROR_PRTF ("ERROR push_torrent_block(): connect_socket() failed.\n");
        return -2;
    }

    INFO_PRTF ("\t[%04.3fs] PUSHING 0x%08x BLOCK %ld TO %s:%d.\n", (double)get_elapsed_msec()/1000, 
        torrent->torrent_hash, block_index, peer->ip, peer->port);

    // Send message.
    if (write_bytes (peer_sock, msg, MSG_LEN) != MSG_LEN)
    {
        ERROR_PRTF ("ERROR push_torrent_block(): send() failed - %d\n", peer_sock);
        close (peer_sock);
        return -2;
    }

    // Send block data.
    if (write_bytes (peer_sock, get_block_ptr (torrent, block_index), BLOCK_SIZE) != BLOCK_SIZE)
    {
        ERROR_PRTF ("ERROR push_torrent_block(): send() failed - %d\n", peer_sock);
        close (peer_sock);
        return -2;
    }


    return 0;
}

// Handle a request for peer list, using push_torrent_peer_list()
// Returns 0 on success, -1 on error.
// Message protocol: REQUEST_TORRENT_PEER_LIST [MY_ENGINE_HASH] [MY_LISTEN_PORT] [TORRENT_HASH]
int handle_request_torrent_peer_list (torrent_engine_t *engine, int peer_sock, 
    peer_data_t *peer, torrent_t *torrent, char *msg_body)
{
    if (peer_sock < 0)
    {
        ERROR_PRTF ("ERROR handle_request_torrent_peer_list(): invalid socket.\n");
        return -1;
    }
    if (engine == NULL || peer == NULL || torrent == NULL)
    {
        ERROR_PRTF ("ERROR handle_request_torrent_peer_list(): invalid argument.\n");
        close (peer_sock);
        return -1;
    }
    close (peer_sock);

    // Check for extra arguments.
    char *val = strtok (msg_body, " ");
    if (val != NULL)
    {
        ERROR_PRTF ("ERROR handle_request_torrent_peer_list(): invalid message.\n");
        return -1;
    }

    INFO_PRTF ("\t[%04.3fs] RECEIVED 0x%08x PEER LIST REQUEST FROM %s:%d.\n", (double)get_elapsed_msec()/1000, 
        torrent->torrent_hash, peer->ip, peer->port);

    // Push peer list.
    if (push_torrent_peer_list (peer, torrent) < -1)
    {
        ERROR_PRTF ("ERROR handle_request_torrent_peer_list(): push_torrent_peer_list() failed.\n");
        return -1;
    }

    return 0;
}

// Handle a request for block info, using push_torrent_block_status()
// Returns 0 on success, -1 on error.
// Message protocol: REQUEST_TORRENT_BLOCK_STATUS [MY_ENGINE_HASH] [MY_LISTEN_PORT] [TORRENT_HASH]
int handle_request_torrent_block_status (torrent_engine_t *engine, int peer_sock, 
    peer_data_t *peer, torrent_t *torrent, char *msg_body)
{
    if (peer_sock < 0)
    {
        ERROR_PRTF ("ERROR handle_request_torrent_block_status(): invalid socket.\n");
        return -1;
    }
    if (engine == NULL || peer == NULL || torrent == NULL)
    {
        ERROR_PRTF ("ERROR handle_request_torrent_block_status(): invalid argument.\n");
        close (peer_sock);
        return -1;
    }
    close (peer_sock);

    // Check for extra arguments.
    char *val = strtok (msg_body, " ");
    if (val != NULL)
    {
        ERROR_PRTF ("ERROR handle_request_torrent_block_status(): invalid message.\n");
        return -1;
    }

    INFO_PRTF ("\t[%04.3fs] RECEIVED 0x%08x BLOCK STATUS REQUEST FROM %s:%d.\n", (double)get_elapsed_msec()/1000, 
        torrent->torrent_hash, peer->ip, peer->port);

    // Push block status.
    if (push_torrent_block_status (peer, torrent) < -1)
    {
        ERROR_PRTF ("ERROR handle_request_torrent_block_status(): push_torrent_block_status() failed.\n");
        return -1;
    }

    return 0;
}

// Handle a request for block, using push_torrent_block()
// Returns 0 on success, -1 on error.
// Message protocol: "REQUEST_TORRENT_BLOCK [MY_ENGINE_HASH] [MY_LISTEN_PORT] [TORRENT_HASH] [BLOCK_INDEX]""
int handle_request_torrent_block (torrent_engine_t *engine, int peer_sock, 
    peer_data_t *peer, torrent_t *torrent, char *msg_body)
{
    if (peer_sock < 0)
    {
        ERROR_PRTF ("ERROR handle_request_torrent_block(): invalid socket.\n");
        return -1;
    }
    if (engine == NULL || peer == NULL || torrent == NULL)
    {
        ERROR_PRTF ("ERROR handle_request_torrent_block(): invalid argument.\n");
        return -1;
    }
    close (peer_sock);

    // Parse block index.
    char *val = strtok (msg_body, " ");
    if (val == NULL)
    {
        ERROR_PRTF ("ERROR handle_request_torrent_block(): invalid message.\n");
        return -1;
    }
    ssize_t block_index = atoi (val);
    if (block_index >= torrent->num_blocks || block_index < 0 || torrent->block_status[block_index] != 1)
    {
        ERROR_PRTF ("ERROR handle_request_torrent_block(): invalid block index.\n");
        return -1;
    }

    // Check for extra arguments.
    val = strtok (msg_body, " ");
    if (val != NULL)
    {
        ERROR_PRTF ("ERROR handle_request_torrent_block(): invalid message.\n");
        return -1;
    }

    INFO_PRTF ("\t[%04.3fs] RECEIVED 0x%08x BLOCK REQUEST FROM %s:%d.\n", (double)get_elapsed_msec()/1000, 
        torrent->torrent_hash, peer->ip, peer->port);
    
    // Push block.
    if (push_torrent_block (peer, torrent, block_index) < -1)
    {
        ERROR_PRTF ("ERROR handle_request_torrent_block(): push_torrent_block() failed.\n");
        return -1;
    }

    return 0;
}

// Handle a push of peer list.
// Returns 0 on success, -1 on error.
// PUSH_TORRENT_PEER_LIST [MY_ENGINE_HASH] [MY_LISTEN_PORT] [TORRENT_HASH] [NUM_PEERS] [PEER_0_IP]:[PEER_0_PORT] [PEE_1_IP]:[PEER_1_PORT] ...
// [PEER_X_IP]:[PEER_X_PORT] is a list of peers, which starts AFTER MSG_LEN bytes in the message.
// Each the list of peers is  [NUM_PEERS] * PEER_LIST_BYTE_PER_PEER bytes long, including the space.
int handle_push_torrent_peer_list (torrent_engine_t *engine, int peer_sock, 
    peer_data_t *peer, torrent_t *torrent, char *msg_body)
{
    if (peer_sock < 0)
    {
        ERROR_PRTF ("ERROR handle_push_torrent_peer_list(): invalid socket.\n");
        return -1;
    }
    if (engine == NULL || peer == NULL || torrent == NULL)
    {
        ERROR_PRTF ("ERROR handle_push_torrent_peer_list(): invalid argument.\n");
        close (peer_sock);
        return -1;
    }

    // Parse num peers.
    char *val = strtok (msg_body, " ");
    if (val == NULL)
    {
        ERROR_PRTF ("ERROR handle_push_torrent_peer_list(): invalid message.\n");
        close (peer_sock);
        return -1;
    }
    ssize_t num_peers = atoi (val);
    if (num_peers < 0)
    {
        ERROR_PRTF ("ERROR handle_push_torrent_peer_list(): invalid num peers.\n");
        close (peer_sock);
        return -1;
    }

    // Read peer list from the socket.
    size_t peer_list_buffer_size = num_peers * PEER_LIST_MAX_BYTE_PER_PEER;
    char *peer_list_buffer = calloc (1, peer_list_buffer_size);
    if (peer_list_buffer == NULL)
    {
        ERROR_PRTF ("ERROR handle_push_torrent_peer_list(): calloc() failed.\n");
        close (peer_sock);
        return -1;
    }
    if (read_bytes (peer_sock, peer_list_buffer, peer_list_buffer_size) != peer_list_buffer_size)
    {
        ERROR_PRTF ("ERROR handle_push_torrent_peer_list(): read() failed.\n");
        close (peer_sock);
        free (peer_list_buffer);
        return -1;
    }
    close (peer_sock);

    // Parse peer list.
    for (size_t i = 0; i < num_peers; i++)
    {
        val = strtok (peer_list_buffer, " ");
        peer_list_buffer = val + strlen(val) + 1;

        char *ip = strtok (val, ":");
        if (ip == NULL)
        {
            ERROR_PRTF ("ERROR handle_push_torrent_peer_list(): invalid peer list.\n");
            free (peer_list_buffer);
            return -1;
        }
        char *port_str = strtok (NULL, "\0");
        if (port_str == NULL)
        {
            ERROR_PRTF ("ERROR handle_push_torrent_peer_list(): invalid peer list.\n");
            free (peer_list_buffer);
            return -1;
        }
        int port = atoi (port_str);
        if (port <= 0 || port > 65535)
        {
            ERROR_PRTF ("ERROR handle_push_torrent_peer_list(): invalid peer port.\n");
            free (peer_list_buffer);
            return -1;
        }

        // Check if peer is already in peer list.
        ssize_t peer_index = find_peer (torrent, ip, port);
        // If peer is not in peer list, add the peer to the peer list.
        if (peer_index < 0)
        {
            if (torrent_add_peer (torrent, ip, port) < 0)
            {
                ERROR_PRTF ("ERROR handle_push_torrent_peer_list(): add_peer() failed.\n");
                free (peer_list_buffer);
                return -1;
            }
        }
    }

    // Clear number of requests of the peer.
    peer->num_requests = 0;

    return 0;
}

// Handle a push of block status.
// Returns 0 on success, -1 on error.
// PUSH_TORRENT_BLOCK_STATUS [MY_ENGINE_HASH] [MY_LISTEN_PORT] [TORRENT_HASH] [BLOCK_STATUS]
// [BLOCK_STATUS] is a binary dump of the torrent's block status, which starts AFTER MSG_LEN bytes in the message.
int handle_push_torrent_block_status (torrent_engine_t *engine, int peer_sock, 
    peer_data_t *peer, torrent_t *torrent, char *msg_body)
{
    if (peer_sock < 0)
    {
        ERROR_PRTF ("ERROR handle_push_torrent_block_status(): invalid socket.\n");
        return -1;
    }
    if (engine == NULL || peer == NULL || torrent == NULL)
    {
        ERROR_PRTF ("ERROR handle_push_torrent_block_status(): invalid argument.\n");
        return -1;
    }

    return 0;
}

// Handle a push of block.
// Check hash of the received block. If it does not match the expected hash, drop the block.
// Returns 0 on success, -1 on error.
// PUSH_TORRENT_BLOCK [MY_ENGINE_HASH] [MY_LISTEN_PORT] [TORRENT_HASH] [BLOCK_INDEX] [BLOCK_DATA]
// [BLOCK_DATA] is a binary dump of the block data, which starts AFTER MSG_LEN bytes in the message.
int handle_push_torrent_block (torrent_engine_t *engine, int peer_sock, 
    peer_data_t *peer, torrent_t *torrent, char *msg_body)
{
    return 0;
}

// Request torrent info from a remote host. Returns 0 on success, -1 on timeout, -2 on error.
// Message protocol: REQUEST_TORRENT_INFO [MY_ENGINE_HASH] [MY_LISTEN_PORT] [TORRENT_HASH]
int request_torrent_info (peer_data_t *peer, torrent_t *torrent)
{
    if (peer == NULL || torrent == NULL)
    {
        ERROR_PRTF ("ERROR request_torrent_info(): invalid argument.\n");
        return -2;
    }

    // Update peer info.
    peer->num_requests++;
    peer->last_torrent_info_request_msec = get_elapsed_msec();

    // Format message.
    char msg[MSG_LEN] = {0};
    sprintf (msg, "REQUEST_TORRENT_INFO 0x%08x %d 0x%08x", torrent->engine->engine_hash,
        torrent->engine->port, torrent->torrent_hash);
    
    // Connect to peer.
    int peer_sock = connect_socket (peer->ip, peer->port);
    if (peer_sock == -1)
    {
        INFO_PRTF ("\t[%04.3fs] REQUESTING 0x%08x INFO FROM %s:%d - TIMEOUT.\n", (double)get_elapsed_msec()/1000, 
            torrent->torrent_hash, peer->ip, peer->port);
        return -1;
    }
    else if (peer_sock < -1)
    {
        ERROR_PRTF ("ERROR request_torrent_info(): connect_socket() failed.\n");
        return -2;
    }

    INFO_PRTF ("\t[%04.3fs] REQUESTING 0x%08x INFO FROM %s:%d.\n", (double)get_elapsed_msec()/1000, 
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
// PUSH_TORRENT_INFO [MY_ENGINE_HASH] [MY_LISTEN_PORT] [TORRENT_HASH] [TORRENT_NAME] [FILE_SIZE] [BLOCK_HASH]
// [BLOCK_HASH] is a binary dump of the torrent's block hashes, which starts AFTER MSG_LEN bytes in the message.
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
    if (peer_sock == -1)
    {
        INFO_PRTF ("\t[%04.3fs] PUSHING 0x%08x INFO TO %s:%d - TIMEOUT.\n", (double)get_elapsed_msec()/1000, 
            torrent->torrent_hash, peer->ip, peer->port);
        return -1;
    }
    else if (peer_sock < -1)
    {
        ERROR_PRTF ("ERROR push_torrent_info(): connect_socket() failed.\n");
        return -2;
    }

    INFO_PRTF ("\t[%04.3fs] PUSHING 0x%08x INFO TO %s:%d.\n", (double)get_elapsed_msec()/1000, 
        torrent->torrent_hash, peer->ip, peer->port);

    // Send message.
    if (write_bytes (peer_sock, msg, MSG_LEN) != MSG_LEN)
    {
        ERROR_PRTF ("ERROR push_torrent_info(): send() failed.\n");
        close (peer_sock);
        return -2;
    }

    // Send block hashes.
    if (write_bytes (peer_sock, torrent->block_hashes, torrent->num_blocks * sizeof(HASH_t)) != 
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
// Message protocol: REQUEST_TORRENT_INFO [MY_ENGINE_HASH] [MY_LISTEN_PORT] [TORRENT_HASH]
int handle_request_torrent_info (torrent_engine_t *engine, int peer_sock, 
    peer_data_t *peer, torrent_t *torrent, char *msg_body)
{
    if (peer_sock < 0)
    {
        ERROR_PRTF ("ERROR handle_request_torrent_info(): invalid socket.\n");
        return -1;
    }
    if (engine == NULL || peer == NULL || torrent == NULL)
    {
        ERROR_PRTF ("ERROR handle_push_torrent_info(): invalid argument.\n");
        close (peer_sock);
        return -1;
    }
    close (peer_sock);

    // Check for extra arguments.
    char *val = strtok (msg_body, " ");
    if (val != NULL)
    {
        ERROR_PRTF ("ERROR handle_request_torrent_info(): invalid message.\n");
        return -1;
    }

    INFO_PRTF ("\t[%04.3fs] RECEIVED REQUEST FOR 0x%08x INFO FROM %s:%d\n", (double)get_elapsed_msec()/1000,
        torrent->torrent_hash, peer->ip, peer->port);

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
// Message protocol: 
// PUSH_TORRENT_INFO [MY_ENGINE_HASH] [MY_LISTEN_PORT] [TORRENT_HASH] [TORRENT_NAME] [FILE_SIZE] [BLOCK_HASH]
// [BLOCK_HASH] is a binary dump of the torrent's block hashes, which starts AFTER MSG_LEN bytes in the message.
int handle_push_torrent_info (torrent_engine_t *engine, int peer_sock, 
    peer_data_t *peer, torrent_t *torrent, char *msg_body)
{
    if (peer_sock < 0)
    {
        ERROR_PRTF ("ERROR handle_push_torrent_info(): invalid socket.\n");
        return -1;
    }
    if (engine == NULL || peer == NULL || torrent == NULL || msg_body == NULL)
    {
        ERROR_PRTF ("ERROR handle_push_torrent_info(): invalid argument.\n");
        close (peer_sock);
        return -1;
    }

    // Parse torrent name.
    char *val = strtok (msg_body, " ");
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

    INFO_PRTF ("\t[%04.3fs] RECEIVED PUSH FOR 0x%08x INFO FROM %s:%d- NAME: %s, SIZE: %ld\n"
        , (double)get_elapsed_msec()/1000, torrent->torrent_hash, peer->ip, peer->port, torrent_name, file_size);

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
    if (read_bytes (peer_sock, torrent->block_hashes, torrent->num_blocks * sizeof(HASH_t)) != 
        torrent->num_blocks * sizeof(HASH_t))
    {
        ERROR_PRTF ("ERROR handle_push_torrent_info(): read_bytes() failed.\n");
        close (peer_sock);
        return -1;
    }
    close (peer_sock);

    // Clear number of requests of the peer.
    peer->num_requests = 0;

    return 0;
}