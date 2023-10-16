// NXC Data Communications torrent_engine.c for torrent
// Written by Jongseok Park (cakeng@snu.ac.kr)
// 2023. 9. 19


#include "torrent_engine.h"
#include "torrent_ui.h"
#include "torrent_utils.h"

int request_torrent_info_ans (peer_data_t *peer, torrent_t *torrent);
int push_torrent_info_ans (peer_data_t *peer, torrent_t *torrent);
int handle_request_torrent_info_ans (torrent_engine_t *engine, int peer_sock, 
    peer_data_t *peer, torrent_t *torrent, char *msg_body);
int handle_push_torrent_info_ans (torrent_engine_t *engine, int peer_sock, 
    peer_data_t *peer, torrent_t *torrent, char *msg_body);
int torrent_client_ans (torrent_engine_t *engine);
int torrent_server_ans (torrent_engine_t *engine);
int listen_socket_ans (int port);
int accept_socket_ans (int listen_sock, struct sockaddr_in *cli_addr, socklen_t *clilen);
int connect_socket_ans (char *server_ip, int port);
int request_torrent_peer_list_ans (peer_data_t *peer, torrent_t *torrent);
int request_torrent_block_status_ans (peer_data_t *peer, torrent_t *torrent);
int request_torrent_block_ans (peer_data_t *peer, torrent_t *torrent, size_t block_index);
int push_torrent_peer_list_ans (peer_data_t *peer, torrent_t *torrent);
int push_torrent_block_status_ans (peer_data_t *peer, torrent_t *torrent); 
int push_torrent_block_ans (peer_data_t *peer, torrent_t *torrent, size_t block_index);
int handle_request_torrent_peer_list_ans (torrent_engine_t *engine, int peer_sock, 
    peer_data_t *peer, torrent_t *torrent, char *msg_body);
int handle_request_torrent_block_status_ans (torrent_engine_t *engine, int peer_sock, 
    peer_data_t *peer, torrent_t *torrent, char *msg_body);
int handle_request_torrent_block_ans (torrent_engine_t *engine, int peer_sock, 
    peer_data_t *peer, torrent_t *torrent, char *msg_body);
int handle_push_torrent_peer_list_ans (torrent_engine_t *engine, int peer_sock, 
    peer_data_t *peer, torrent_t *torrent, char *msg_body);
int handle_push_torrent_block_status_ans (torrent_engine_t *engine, int peer_sock, 
    peer_data_t *peer, torrent_t *torrent, char *msg_body);
int handle_push_torrent_block_ans (torrent_engine_t *engine, int peer_sock, 
    peer_data_t *peer, torrent_t *torrent, char *msg_body);

void *request_torrent_info_thread_wrapper_ans (void *_request_wrapper_data)
{
    request_wrapper_data_t *request_wrapper_data = (request_wrapper_data_t *)_request_wrapper_data;
    request_torrent_info_ans (request_wrapper_data->peer, request_wrapper_data->torrent);
    free (request_wrapper_data);
    return NULL;
}

void *request_torrent_peer_list_thread_wrapper_ans (void *_request_wrapper_data)
{
    request_wrapper_data_t *request_wrapper_data = (request_wrapper_data_t *)_request_wrapper_data;
    request_torrent_peer_list_ans (request_wrapper_data->peer, request_wrapper_data->torrent);
    free (request_wrapper_data);
    return NULL;
}

void *request_torrent_block_status_thread_wrapper_ans (void *_request_wrapper_data)
{
    request_wrapper_data_t *request_wrapper_data = (request_wrapper_data_t *)_request_wrapper_data;
    request_torrent_block_status_ans (request_wrapper_data->peer, request_wrapper_data->torrent);
    free (request_wrapper_data);
    return NULL;
}

void *request_torrent_block_thread_wrapper_ans (void *_request_wrapper_data)
{
    request_wrapper_data_t *request_wrapper_data = (request_wrapper_data_t *)_request_wrapper_data;
    request_torrent_block_ans (request_wrapper_data->peer, request_wrapper_data->torrent, 
            request_wrapper_data->block_index);
    free (request_wrapper_data);
    return NULL;
}

int request_torrent_info_thread_ans (peer_data_t *peer, torrent_t *torrent)
{
    pthread_t request_thread;
    pthread_attr_t attr;
    request_wrapper_data_t *request_wrapper_data = calloc (1, sizeof (request_wrapper_data_t));
    if (request_wrapper_data == NULL)
    {
        ERROR_PRTF ("ERROR request_torrent_info_thread(): calloc failed. (ERRNO %d:%s)\n", errno, strerror(errno));
        return -1;
    }
    request_wrapper_data->peer = peer;
    request_wrapper_data->torrent = torrent;
    request_wrapper_data->block_index = 0;
    if (pthread_attr_init(&attr) != 0 )
    {
        ERROR_PRTF ("ERROR request_torrent_info_thread(): pthread_attr_init failed. (ERRNO %d:%s)\n", errno, strerror(errno));
        return -1;
    }
    if (pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED) != 0 )
    {
        ERROR_PRTF ("ERROR request_torrent_info_thread(): pthread_attr_setdetachstate failed. (ERRNO %d:%s)\n", errno, strerror(errno));
        return -1;
    }
    if (pthread_create (&request_thread, &attr, request_torrent_info_thread_wrapper_ans, 
            (void *)request_wrapper_data) != 0)
    {
        ERROR_PRTF ("ERROR request_torrent_info_thread(): pthread_create failed. (ERRNO %d:%s)\n", errno, strerror(errno));
        return -1;
    }
    if (pthread_attr_destroy(&attr) != 0)
    {
        ERROR_PRTF ("ERROR request_torrent_info_thread(): pthread_attr_destroy failed. (ERRNO %d:%s)\n", errno, strerror(errno));
        return -1;
    }

    return 0;
}

int request_torrent_peer_list_thread_ans (peer_data_t *peer, torrent_t *torrent)
{
    pthread_t request_thread;
    pthread_attr_t attr;
    request_wrapper_data_t *request_wrapper_data = calloc (1, sizeof (request_wrapper_data_t));
    if (request_wrapper_data == NULL)
    {
        ERROR_PRTF ("ERROR request_torrent_peer_list_thread(): calloc failed. (ERRNO %d:%s)\n", errno, strerror(errno));
        return -1;
    }
    request_wrapper_data->peer = peer;
    request_wrapper_data->torrent = torrent;
    request_wrapper_data->block_index = 0;
    if (pthread_attr_init(&attr) != 0 )
    {
        ERROR_PRTF ("ERROR request_torrent_peer_list_thread(): pthread_attr_init failed. (ERRNO %d:%s)\n", errno, strerror(errno));
        return -1;
    }
    if (pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED) != 0 )
    {
        ERROR_PRTF ("ERROR request_torrent_peer_list_thread(): pthread_attr_setdetachstate failed. (ERRNO %d:%s)\n", errno, strerror(errno));
        return -1;
    }
    if (pthread_create (&request_thread, NULL, request_torrent_peer_list_thread_wrapper_ans, 
            (void *)request_wrapper_data) != 0)
    {
        ERROR_PRTF ("ERROR request_torrent_peer_list_thread(): pthread_create failed. (ERRNO %d:%s)\n", errno, strerror(errno));
        return -1;
    }
    if (pthread_attr_destroy(&attr) != 0)
    {
        ERROR_PRTF ("ERROR request_torrent_peer_list_thread(): pthread_attr_destroy failed. (ERRNO %d:%s)\n", errno, strerror(errno));
        return -1;
    }
    return 0;
}

int request_torrent_block_status_thread_ans (peer_data_t *peer, torrent_t *torrent)
{
    pthread_t request_thread;
    pthread_attr_t attr;
    request_wrapper_data_t *request_wrapper_data = calloc (1, sizeof (request_wrapper_data_t));
    if (request_wrapper_data == NULL)
    {
        ERROR_PRTF ("ERROR request_torrent_block_status_thread(): calloc failed. (ERRNO %d:%s)\n", errno, strerror(errno));
        return -1;
    }
    request_wrapper_data->peer = peer;
    request_wrapper_data->torrent = torrent;
    request_wrapper_data->block_index = 0;
    if (pthread_attr_init(&attr) != 0 )
    {
        ERROR_PRTF ("ERROR request_torrent_block_status_thread(): pthread_attr_init failed. (ERRNO %d:%s)\n", errno, strerror(errno));
        return -1;
    }
    if (pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED) != 0 )
    {
        ERROR_PRTF ("ERROR request_torrent_block_status_thread(): pthread_attr_setdetachstate failed. (ERRNO %d:%s)\n", errno, strerror(errno));
        return -1;
    }
    if (pthread_create (&request_thread, NULL, request_torrent_block_status_thread_wrapper_ans, 
            (void *)request_wrapper_data) != 0)
    {
        ERROR_PRTF ("ERROR request_torrent_block_status_thread(): pthread_create failed. (ERRNO %d:%s)\n", errno, strerror(errno));
        return -1;
    }
    if (pthread_attr_destroy(&attr) != 0)
    {
        ERROR_PRTF ("ERROR request_torrent_block_status_thread(): pthread_attr_destroy failed. (ERRNO %d:%s)\n", errno, strerror(errno));
        return -1;
    }
    return 0;
}

int request_torrent_block_thread_ans (peer_data_t *peer, torrent_t *torrent, size_t block_index)
{
    pthread_t request_thread;
    pthread_attr_t attr;
    request_wrapper_data_t *request_wrapper_data = calloc (1, sizeof (request_wrapper_data_t));
    if (request_wrapper_data == NULL)
    {
        ERROR_PRTF ("ERROR request_torrent_block_thread(): calloc failed. (ERRNO %d:%s)\n", errno, strerror(errno));
        return -1;
    }
    request_wrapper_data->peer = peer;
    request_wrapper_data->torrent = torrent;
    request_wrapper_data->block_index = block_index;
    if (pthread_attr_init(&attr) != 0 )
    {
        ERROR_PRTF ("ERROR request_torrent_block_thread(): pthread_attr_init failed. (ERRNO %d:%s)\n", errno, strerror(errno));
        return -1;
    }
    if (pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED) != 0 )
    {
        ERROR_PRTF ("ERROR request_torrent_block_thread(): pthread_attr_setdetachstate failed. (ERRNO %d:%s)\n", errno, strerror(errno));
        return -1;
    }
    if (pthread_create (&request_thread, NULL, request_torrent_block_thread_wrapper_ans, 
            (void *)request_wrapper_data) != 0)
    {
        ERROR_PRTF ("ERROR request_torrent_block_thread(): pthread_create failed. (ERRNO %d:%s)\n", errno, strerror(errno));
        return -1;
    }
    if (pthread_attr_destroy(&attr) != 0)
    {
        ERROR_PRTF ("ERROR request_torrent_block_thread(): pthread_attr_destroy failed. (ERRNO %d:%s)\n", errno, strerror(errno));
        return -1;
    }
    return 0;
}


// The implementations for below functions are provided as a reference for the project.

// Request torrent info from a remote host. Returns 0 on success, -1 on timeout, -2 on error.
// Message protocol: REQUEST_TORRENT_INFO [MY_ENGINE_HASH] [MY_LISTEN_PORT] [TORRENT_HASH]
int request_torrent_info_ans (peer_data_t *peer, torrent_t *torrent)
{
    if (peer == NULL || torrent == NULL)
    {
        ERROR_PRTF ("ERROR request_torrent_info_ans(): invalid argument.\n");
        return -2;
    }

    // Update peer info.
    peer->last_torrent_info_request_msec = get_elapsed_msec();

    // Format message.
    char msg[MSG_LEN] = {0};
    sprintf (msg, "REQUEST_TORRENT_INFO 0x%08x %d 0x%08x", torrent->engine->engine_hash,
        torrent->engine->port, torrent->torrent_hash);
    
    // Connect to peer.
    int peer_sock = connect_socket_ans (peer->ip, peer->port);
    if (peer_sock == -1)
    {
        INFO_PRTF ("REQUESTING 0x%08x INFO FROM %s:%d - TIMEOUT.\n", torrent->torrent_hash, peer->ip, peer->port);
        return -1;
    }
    else if (peer_sock < -1)
    {
        ERROR_PRTF ("ERROR request_torrent_info_ans(): connect_socket_ans() failed.\n");
        return -2;
    }

    INFO_PRTF ("REQUESTING 0x%08x INFO FROM %s:%d.\n", torrent->torrent_hash, peer->ip, peer->port);

    // Send message.
    if (write_bytes (peer_sock, msg, MSG_LEN) != MSG_LEN)
    {
        ERROR_PRTF ("ERROR request_torrent_info_ans(): write_bytes() failed - %d\n", peer_sock);
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
int push_torrent_info_ans (peer_data_t *peer, torrent_t *torrent)
{
    if (peer == NULL || torrent == NULL)
    {
        ERROR_PRTF ("ERROR push_torrent_info_ans(): invalid argument.\n");
        return -2;
    }

    // Format message.
    char msg[MSG_LEN] = {0};
    sprintf (msg, "PUSH_TORRENT_INFO 0x%08x %d 0x%08x %s %ld", torrent->engine->engine_hash,
        torrent->engine->port, torrent->torrent_hash, torrent->torrent_name, torrent->file_size);

    // Connect to peer.
    int peer_sock = connect_socket_ans (peer->ip, peer->port);
    if (peer_sock == -1)
    {
        INFO_PRTF ("PUSHING 0x%08x INFO TO %s:%d - TIMEOUT.\n", torrent->torrent_hash, peer->ip, peer->port);
        return -1;
    }
    else if (peer_sock < -1)
    {
        ERROR_PRTF ("ERROR push_torrent_info_ans(): connect_socket_ans() failed.\n");
        return -2;
    }

    INFO_PRTF ("PUSHING 0x%08x INFO TO %s:%d.\n", torrent->torrent_hash, peer->ip, peer->port);

    // Send message.
    if (write_bytes (peer_sock, msg, MSG_LEN) != MSG_LEN)
    {
        ERROR_PRTF ("ERROR push_torrent_info_ans(): write_bytes() failed.\n");
        close (peer_sock);
        return -2;
    }

    // Send block hashes.
    if (write_bytes (peer_sock, torrent->block_hashes, torrent->num_blocks * sizeof(HASH_t)) != 
        torrent->num_blocks * sizeof(HASH_t))
    {
        ERROR_PRTF ("ERROR push_torrent_info_ans(): write_bytes() failed.\n");
        close (peer_sock);
        return -2;
    }
    close (peer_sock);

    return 0;
}

// Handle a request for torrent info, using push_torrent_info_ans()
// Returns 0 on success, -1 on error.
// Message protocol: REQUEST_TORRENT_INFO [MY_ENGINE_HASH] [MY_LISTEN_PORT] [TORRENT_HASH]
int handle_request_torrent_info_ans (torrent_engine_t *engine, int peer_sock, 
    peer_data_t *peer, torrent_t *torrent, char *msg_body)
{
    if (peer_sock < 0)
    {
        ERROR_PRTF ("ERROR handle_request_torrent_info_ans(): invalid socket.\n");
        return -1;
    }
    if (engine == NULL || peer == NULL || torrent == NULL)
    {
        ERROR_PRTF ("ERROR handle_push_torrent_info_ans(): invalid argument.\n");
        close (peer_sock);
        return -1;
    }
    close (peer_sock);

    // Check for extra arguments.
    char *val = strtok (msg_body, " ");
    if (val != NULL)
    {
        ERROR_PRTF ("ERROR handle_request_torrent_info_ans(): invalid message.\n");
        return -1;
    }

    INFO_PRTF ("RECEIVED REQUEST FOR 0x%08x INFO FROM %s:%d\n", torrent->torrent_hash, peer->ip, peer->port);

    // If torrent info exists, Push torrent info to peer.
    if (is_torrent_info_set (torrent) == 1 && push_torrent_info_ans (peer, torrent) < -1)
    {
        ERROR_PRTF ("ERROR handle_request_torrent_info_ans(): push_torrent_info_ans() failed.\n");
        return -1;
    }

    return 0;
}

// Handle a push of torrent info.
// Returns 0 on success, -1 on error.
// Message protocol: 
// PUSH_TORRENT_INFO [MY_ENGINE_HASH] [MY_LISTEN_PORT] [TORRENT_HASH] [TORRENT_NAME] [FILE_SIZE] [BLOCK_HASH]
// [BLOCK_HASH] is a binary dump of the torrent's block hashes, which starts AFTER MSG_LEN bytes in the message.
int handle_push_torrent_info_ans (torrent_engine_t *engine, int peer_sock, 
    peer_data_t *peer, torrent_t *torrent, char *msg_body)
{
    if (peer_sock < 0)
    {
        ERROR_PRTF ("ERROR handle_push_torrent_info_ans(): invalid socket.\n");
        return -1;
    }
    if (engine == NULL || peer == NULL || torrent == NULL || msg_body == NULL)
    {
        ERROR_PRTF ("ERROR handle_push_torrent_info_ans(): invalid argument.\n");
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
        ERROR_PRTF ("ERROR handle_push_torrent_info_ans(): invalid message.\n");
        close (peer_sock);
        return -1;
    }

    INFO_PRTF ("RECEIVED PUSH FOR 0x%08x INFO FROM %s:%d- NAME: %s, SIZE: %ld\n"
        , torrent->torrent_hash, peer->ip, peer->port, torrent_name, file_size);

    // Check if torrent already has info.
    if (is_torrent_info_set (torrent) == 1)
    {
        // ERROR_PRTF ("ERROR handle_push_torrent_info_ans(): torrent already has info.\n");
        close (peer_sock);
        return -1;
    }

    // Update torrent info.
    if (set_torrent_info (torrent, torrent_name, file_size) < 0)
    {
        ERROR_PRTF ("ERROR handle_push_torrent_info_ans(): set_torrent_info() failed.\n");
        close (peer_sock);
        return -1;
    }

    // Read block hashes.
    if (read_bytes (peer_sock, torrent->block_hashes, torrent->num_blocks * sizeof(HASH_t)) != 
        torrent->num_blocks * sizeof(HASH_t))
    {
        ERROR_PRTF ("ERROR handle_push_torrent_info_ans(): read_bytes() failed.\n");
        close (peer_sock);
        return -1;
    }
    close (peer_sock);

    return 0;
}

// Client Routine.
// Returns 0 on success, -1 on error.
int torrent_client_ans (torrent_engine_t *engine)
{
    if (engine == NULL)
    {
        ERROR_PRTF ("ERROR torrent_client_ans(): invalid argument.\n");
        return -1;
    }

    // Iterate through all torrents and take action.
    for (size_t i = 0; i < engine->num_torrents; i++)
    {
        torrent_t *torrent = engine->torrents[i];
        if (torrent == NULL)
        {
            ERROR_PRTF ("ERROR torrent_client_ans(): invalid torrent.\n");
            return -1;
        }
        
        // If torrent info has been received...
        if (is_torrent_info_set (torrent) == 1)
        {
            // If TORRENT_SAVE_INTERVAL_MSEC has passed since last save, save the torrent.
            if (get_elapsed_msec() - torrent->last_torrent_save_msec > TORRENT_SAVE_INTERVAL_MSEC)
            {
                if (save_torrent_as_file (torrent) < 0)
                {
                    ERROR_PRTF ("ERROR torrent_client_ans(): save_torrent_as_file() failed.\n");
                    return -1;
                }
            }
            // If RESET_BLOCK_STATUS_INTERVAL_MSEC has passed since last reset, reset block status.
            if (get_elapsed_msec() - torrent->last_block_status_reset_msec > RESET_BLOCK_STATUS_INTERVAL_MSEC)
            {
                INFO_PRTF ("RESETING 0x%08x BLOCK STATUS.\n", torrent->torrent_hash);
                for (size_t j = 0; j < torrent->num_blocks; j++)
                    if (get_block_status (torrent, j) == B_REQUESTED)
                        torrent->block_status[j] = B_MISSING;
                torrent->last_block_status_reset_msec = get_elapsed_msec();
            }
        }
        
        // Iterate through peers.
        for (size_t peer_idx = 0; peer_idx < torrent->num_peers; peer_idx++)
        {
            peer_data_t *peer = torrent->peers[peer_idx];
            // If torrent info has not been received, request the info from the peer.
            if (is_torrent_info_set (torrent) == 0)
            {
                // If REQUEST_TORRENT_INFO_INTERVAL_MSEC has passed since last request, request torrent info.
                // Make sure to use request_torrent_info_thread() instead of request_torrent_info_ans().
                if (get_elapsed_msec() - peer->last_torrent_info_request_msec > REQUEST_TORRENT_INFO_INTERVAL_MSEC)
                    request_torrent_info_thread_ans (peer, torrent);
            }
            else
            {
                // If REQUEST_PEER_LIST_INTERVAL_MSEC has passed since last request, request peer list.
                // Make sure to use request_torrent_peer_list_thread() instead of request_torrent_peer_list_ans().
                if (get_elapsed_msec() - peer->last_peer_list_request_msec > REQUEST_PEER_LIST_INTERVAL_MSEC)
                    request_torrent_peer_list_thread_ans (peer, torrent);
                // If REQUEST_BLOCK_STATUS_INTERVAL_MSEC has passed since last request, request block status.
                // Make sure to use request_torrent_block_status_thread() instead of request_torrent_block_status_ans().
                if (get_elapsed_msec() - peer->last_block_status_request_msec > REQUEST_BLOCK_STATUS_INTERVAL_MSEC)
                    request_torrent_block_status_thread_ans (peer, torrent);
                // If REQUEST_BLOCK_INTERVAL_MSEC has passed since last request, request a block.
                // Make sure to use request_torrent_block_thread() instead of request_torrent_block_ans().
                if (get_elapsed_msec() - peer->last_block_request_msec > REQUEST_BLOCK_INTERVAL_MSEC)
                {            
                    // Get a random missing block that the peer has.
                    ssize_t block_index = get_rand_missing_block_that_peer_has (torrent, peer);
                    if (block_index >= 0)
                        request_torrent_block_thread_ans (peer, torrent, block_index);
                    else if (block_index < -1)
                    {
                        ERROR_PRTF ("ERROR torrent_client_ans(): get_rand_missing_block() failed.\n");
                        return -1;
                    }
                }
            }
        }
    }

    return 0;
}

// Server Routine.
// Returns 0 on success, -1 on error.
int torrent_server_ans (torrent_engine_t *engine)
{
    if (engine == NULL)
    {
        ERROR_PRTF ("ERROR torrent_server_ans(): invalid argument.\n");
        return -1;
    }

    // Accept incoming connections.
    // If there is a connection, handle it.
    // Differentiate between errors and timeouts.
    struct sockaddr_in peer_addr_info;
    socklen_t peer_addr_info_len = sizeof(peer_addr_info);
    int peer_sock = accept_socket_ans (engine->listen_sock, &peer_addr_info, &peer_addr_info_len);
    if (peer_sock < -1)
    {
        ERROR_PRTF ("ERROR torrent_server_ans(): accept_socket_ans() failed.\n");
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
        ERROR_PRTF ("ERROR torrent_server_ans(): read_bytes() failed.\n");
        close (peer_sock);
        return -1;
    }

    INFO_PRTF ("RECEIVED \"%s\" from peer %s:%d.\n", msg, peer_ip, peer_con_port);

    // Parse command from message.
    char *cmd = strtok (msg, " ");
    if (cmd == NULL)
    {
        ERROR_PRTF ("ERROR torrent_server_ans(): invalid message.\n");
        close (peer_sock);
        return -1;
    }
    
    // Parse peer engine hash from message.
    // If the peer engine hash is the same as the local engine hash, ignore the message.
    char *peer_engine_hash = strtok (NULL, " ");
    if (peer_engine_hash == NULL || strlen(peer_engine_hash) != 10)
    {
        ERROR_PRTF ("ERROR torrent_server_ans(): invalid message.\n");
        close (peer_sock);
        return -1;
    }
    else if (strtoul (peer_engine_hash, NULL, 16) == engine->engine_hash)
    {
        ERROR_PRTF ("ERROR torrent_server_ans(): received message from self.\n");
        close (peer_sock);
        return -1;
    }

    // Parse peer listen port from message.
    char *peer_listen_port_str = strtok (NULL, " ");
    if (peer_listen_port_str == NULL)
    {
        ERROR_PRTF ("ERROR torrent_server_ans(): invalid message.\n");
        close (peer_sock);
        return -1;
    }
    int peer_listen_port = atoi (peer_listen_port_str);
    if (peer_listen_port < 0 || peer_listen_port >= 65536)
    {
        ERROR_PRTF ("ERROR torrent_server_ans(): invalid port number.\n");
        close (peer_sock);
        return -1;
    }

    // Parse peer torrent hash from message.
    char *peer_torrent_hash_str = strtok (NULL, " ");
    if (peer_torrent_hash_str == NULL || strlen(peer_torrent_hash_str) != 10)
    {
        ERROR_PRTF ("ERROR torrent_server_ans(): invalid message.\n");
        close (peer_sock);
        return -1;
    }
    HASH_t peer_torrent_hash = strtoul (peer_torrent_hash_str, NULL, 16);
    ssize_t torrent_index = find_torrent (engine, peer_torrent_hash);
    if (torrent_index < 0)
    {
        // ERROR_PRTF ("ERROR torrent_server_ans(): invalid torrent hash.\n");
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
            ERROR_PRTF ("ERROR torrent_server_ans(): torrent_add_peer() failed.\n");
            close (peer_sock);
            return -1;
        }
    }
    peer_data_t *peer = torrent->peers[peer_idx];

    char *msg_body = strtok (NULL, "\0");

    // Take action based on message.
    if (strncmp (cmd, "REQUEST_TORRENT_INFO", 20) == 0)
        handle_request_torrent_info_ans (engine, peer_sock, peer, torrent, msg_body);
    else if (strncmp (cmd, "PUSH_TORRENT_INFO", 17) == 0)
        handle_push_torrent_info_ans (engine, peer_sock, peer, torrent, msg_body);
    else if (strncmp (cmd, "REQUEST_TORRENT_PEER_LIST", 25) == 0)
        handle_request_torrent_peer_list_ans (engine, peer_sock, peer, torrent, msg_body);
    else if (strncmp (cmd, "PUSH_TORRENT_PEER_LIST", 22) == 0)
        handle_push_torrent_peer_list_ans (engine, peer_sock, peer, torrent, msg_body);
    else if (strncmp (cmd, "REQUEST_TORRENT_BLOCK_STATUS", 28) == 0)
        handle_request_torrent_block_status_ans (engine, peer_sock, peer, torrent, msg_body);
    else if (strncmp (cmd, "PUSH_TORRENT_BLOCK_STATUS", 25) == 0)
        handle_push_torrent_block_status_ans (engine, peer_sock, peer, torrent, msg_body);
    else if (strncmp (cmd, "REQUEST_TORRENT_BLOCK", 21) == 0)
        handle_request_torrent_block_ans (engine, peer_sock, peer, torrent, msg_body);
    else if (strncmp (cmd, "PUSH_TORRENT_BLOCK", 18) == 0)
        handle_push_torrent_block_ans (engine, peer_sock, peer, torrent, msg_body);
    else
    {
        ERROR_PRTF ("ERROR torrent_server_ans(): invalid message.\n");
        return -1;
    }
    
    return 0;
}

// Open a socket and listen for incoming connections. 
// Returns the socket file descriptor on success, -1 on error.
int listen_socket_ans (int port)
{
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) 
    {
        ERROR_PRTF ("ERROR listen_socket_ans(): socket() failed.\n");
        return -1;
    }
    if (port < 0 || port >= 65536)
    {
        ERROR_PRTF ("ERROR listen_socket_ans(): invalid port number.\n");
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
        ERROR_PRTF ("ERROR listen_socket_ans(): setsockopt() failed. (ERRNO %d:%s)\n", errno, strerror(errno));
        close (sockfd);
        return -1;
    }
    if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
    {
        ERROR_PRTF ("ERROR listen_socket_ans(): bind() failed. (ERRNO %d:%s)\n", errno, strerror(errno));
        close (sockfd);
        return -1;
    }
    if (listen(sockfd, MAX_QUEUED_CONNECTIONS) < 0)
    {
        ERROR_PRTF ("ERROR listen_socket_ans(): listen() failed. (ERRNO %d:%s)\n", errno, strerror(errno));
        close (sockfd);
        return -1;
    }
    return sockfd;
}

// Accept an incoming connection with a timeout. 
// Returns the connected socket file descriptor on success, -2 on error, -1 on timeout.
// MUST have a timeout of TIMEOUT_MSEC msec.
// HINT: Use poll(). See kbhit() in torrent_util.c for an example on using poll().
int accept_socket_ans (int listen_sock, struct sockaddr_in *cli_addr, socklen_t *clilen)
{
    struct pollfd fds;
    fds.fd = listen_sock;
    fds.events = POLLIN;
    int ret = poll(&fds, 1, TIMEOUT_MSEC);

    if (ret == 0) 
    {
        // ERROR_PRTF ("ERROR accept_socket_ans(): poll() timeout.\n");
        return -1;
    }
    else if (ret < 0) 
    {
        ERROR_PRTF ("ERROR accept_socket_ans(): poll() failed. (ERRNO %d:%s)\n", errno, strerror(errno));
        return -2;
    }

    int sockfd = accept(listen_sock, (struct sockaddr *) cli_addr, clilen);
    if (sockfd < 0) 
    {
        ERROR_PRTF ("ERROR accept_socket_ans(): accept() failed. (ERRNO %d:%s)\n", errno, strerror(errno));
        return -2;
    }

    return sockfd;
}

// Connect to a server.
// Returns the socket file descriptor on success, -2 on error, -1 on timeout.
// MUST have a timeout of TIMEOUT_MSEC msec.
// HINT: Use poll(). See kbhit() in torrent_utils.c for an example on using poll().
int connect_socket_ans (char *server_ip, int port)
{
    int sockfd;
    struct sockaddr_in serv_addr;
    
    // Create a non-blocking socket.
    sockfd = socket (AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) 
    {
        ERROR_PRTF ("ERROR connect_socket_ans(): socket() failed. (ERRNO %d:%s)\n", errno, strerror(errno));
        return -2;
    }
    int ret = fcntl(sockfd, F_SETFL, O_NONBLOCK);
    if (ret < 0) 
    {
        ERROR_PRTF ("ERROR connect_socket_ans(): fcntl() failed. (ERRNO %d:%s)\n", errno, strerror(errno));
        return -2;
    }
    // Allow reuse of local addresses.
    int val = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val));
    if (sockfd == -1)
    {
        ERROR_PRTF ("ERROR connect_socket_ans(): setsockopt() failed. (ERRNO %d:%s)\n", errno, strerror(errno));
        return -2;
    }
    // Disable Nagle's algorithm.
    val = 1;
    setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY, &val, sizeof(val));
    if (sockfd == -1)
    {
        ERROR_PRTF ("ERROR connect_socket_ans(): setsockopt() failed. (ERRNO %d:%s)\n", errno, strerror(errno));
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
    ret = poll(&fds, 1, TIMEOUT_MSEC);
    if (ret == 0) 
    {
        // ERROR_PRTF ("ERROR connect_socket_ans(): poll() timeout.\n");
        close (sockfd);
        return -1;
    }
    else if (ret < 0) 
    {
        ERROR_PRTF ("ERROR connect_socket_ans(): poll() failed. (ERRNO %d:%s)\n", errno, strerror(errno));
        close (sockfd);
        return -2;
    }

    // Check for errors.
    int err;
    socklen_t len = sizeof(err);
    if (getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &err, &len) < 0)
    {
        ERROR_PRTF ("ERROR connect_socket_ans(): getsockopt() failed. (ERRNO %d:%s)\n", errno, strerror(errno));
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

// Request peer's peer list from a peer. 
// Returns 0 on success, -1 on timeout, -2 on error.
// Message protocol: REQUEST_TORRENT_PEER_LIST [MY_ENGINE_HASH] [MY_LISTEN_PORT] [TORRENT_HASH]
int request_torrent_peer_list_ans (peer_data_t *peer, torrent_t *torrent)
{
    if (peer == NULL || torrent == NULL)
    {
        ERROR_PRTF ("ERROR request_torrent_peer_list_ans(): invalid argument.\n");
        return -2;
    }

    // Update peer info.
    peer->last_peer_list_request_msec = get_elapsed_msec();

    // Format message.
    char msg[MSG_LEN] = {0};
    sprintf (msg, "REQUEST_TORRENT_PEER_LIST 0x%08x %d 0x%08x", torrent->engine->engine_hash,
        torrent->engine->port, torrent->torrent_hash);

    // Connect to peer.
    int peer_sock = connect_socket_ans (peer->ip, peer->port);
    if (peer_sock == -1)
    {
        INFO_PRTF ("REQUESTING 0x%08x PEER LIST FROM %s:%d - TIMEOUT.\n", torrent->torrent_hash, peer->ip, peer->port);
        return -1;
    }
    else if (peer_sock < -1)
    {
        ERROR_PRTF ("ERROR request_torrent_peer_list_ans(): connect_socket_ans() failed.\n");
        return -2;
    }

    INFO_PRTF ("REQUESTING 0x%08x PEER LIST FROM %s:%d.\n", torrent->torrent_hash, peer->ip, peer->port);

    // Send message.
    if (write_bytes (peer_sock, msg, MSG_LEN) != MSG_LEN)
    {
        ERROR_PRTF ("ERROR request_torrent_peer_list_ans(): write_bytes() failed - %d\n", peer_sock);
        close (peer_sock);
        return -2;
    }
    close (peer_sock);

    return 0;
}

// Request peer's block info from a peer. 
// Returns 0 on success, -1 on timeout, -2 on error.
// Message protocol: REQUEST_TORRENT_BLOCK_STATUS [MY_ENGINE_HASH] [MY_LISTEN_PORT] [TORRENT_HASH]
int request_torrent_block_status_ans (peer_data_t *peer, torrent_t *torrent)
{
    if (peer == NULL || torrent == NULL)
    {
        ERROR_PRTF ("ERROR request_torrent_block_status_ans(): invalid argument.\n");
        return -2;
    }

    // Update peer info.
    peer->last_block_status_request_msec = get_elapsed_msec();

    // Format message.
    char msg[MSG_LEN] = {0};
    sprintf (msg, "REQUEST_TORRENT_BLOCK_STATUS 0x%08x %d 0x%08x", torrent->engine->engine_hash,
        torrent->engine->port, torrent->torrent_hash);

    // Connect to peer.
    int peer_sock = connect_socket_ans (peer->ip, peer->port);
    if (peer_sock == -1)
    {
        INFO_PRTF ("REQUESTING 0x%08x BLOCK STATUS FROM %s:%d - TIMEOUT.\n", torrent->torrent_hash, peer->ip, peer->port);
        return -1;
    }
    else if (peer_sock < -1)
    {
        ERROR_PRTF ("ERROR request_torrent_block_status_ans(): connect_socket_ans() failed.\n");
        return -2;
    }

    INFO_PRTF ("REQUESTING 0x%08x BLOCK STATUS FROM %s:%d.\n", torrent->torrent_hash, peer->ip, peer->port);

    // Send message.
    if (write_bytes (peer_sock, msg, MSG_LEN) != MSG_LEN)
    {
        ERROR_PRTF ("ERROR request_torrent_block_status_ans(): write_bytes() failed - %d\n", peer_sock);
        close (peer_sock);
        return -2;
    }
    close (peer_sock);

    return 0;
}

// Request a block from a peer. 
// Returns 0 on success, -1 on timeout, -2 on error.
// Message protocol: REQUEST_TORRENT_BLOCK [MY_ENGINE_HASH] [MY_LISTEN_PORT] [TORRENT_HASH] [BLOCK_INDEX]
int request_torrent_block_ans (peer_data_t *peer, torrent_t *torrent, size_t block_index)
{
    if (peer == NULL || torrent == NULL)
    {
        ERROR_PRTF ("ERROR request_torrent_block_ans(): invalid argument.\n");
        return -2;
    }

    // Update peer info.
    peer->last_block_request_msec = get_elapsed_msec();

    // Format message.
    char msg[MSG_LEN] = {0};
    sprintf (msg, "REQUEST_TORRENT_BLOCK 0x%08x %d 0x%08x %ld", torrent->engine->engine_hash,
        torrent->engine->port, torrent->torrent_hash, block_index);

    // Connect to peer.
    int peer_sock = connect_socket_ans (peer->ip, peer->port);
    if (peer_sock == -1)
    {
        INFO_PRTF ("REQUESTING 0x%08x BLOCK %ld FROM %s:%d - TIMEOUT.\n", torrent->torrent_hash, block_index, peer->ip, peer->port);
        return -1;
    }
    else if (peer_sock < -1)
    {
        ERROR_PRTF ("ERROR request_torrent_block_ans(): connect_socket_ans() failed.\n");
        return -2;
    }

    INFO_PRTF ("REQUESTING 0x%08x BLOCK %ld FROM %s:%d.\n", torrent->torrent_hash, block_index, peer->ip, peer->port);
    
    // Send message.
    if (write_bytes (peer_sock, msg, MSG_LEN) != MSG_LEN)
    {
        ERROR_PRTF ("ERROR request_torrent_block_ans(): write_bytes() failed - %d\n", peer_sock);
        close (peer_sock);
        return -2;
    }
    close (peer_sock);

    // Set block status to requested.
    if (get_block_status (torrent, block_index) == B_MISSING)
        torrent->block_status[block_index] = B_REQUESTED;

    return 0;
}

// Push my list of peers to a peer. 
// Returns 0 on success, -1 on timeout, -2 on error.
// Message protocol: 
// PUSH_TORRENT_PEER_LIST [MY_ENGINE_HASH] [MY_LISTEN_PORT] [TORRENT_HASH] [NUM_PEERS] [PEER_0_IP]:[PEER_0_PORT] [PEE_1_IP]:[PEER_1_PORT] ...
// [PEER_X_IP]:[PEER_X_PORT] is a list of peers, which starts AFTER MSG_LEN bytes in the message.
// Each the list of peers is  [NUM_PEERS] * PEER_LIST_BYTE_PER_PEER bytes long, including the space.
// Make sure not to send the IP and port of the receiving peer back to itself.
int push_torrent_peer_list_ans (peer_data_t *peer, torrent_t *torrent)
{
    if (peer == NULL || torrent == NULL)
    {
        ERROR_PRTF ("ERROR push_torrent_peer_list_ans(): invalid argument.\n");
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
    int peer_sock = connect_socket_ans (peer->ip, peer->port);
    if  (peer_sock == -1)
    {
        INFO_PRTF ("PUSHING 0x%08x PEER LIST TO %s:%d - TIMEOUT.\n", torrent->torrent_hash, peer->ip, peer->port);
        return -1;
    }
    else if (peer_sock < -1)
    {
        ERROR_PRTF ("ERROR push_torrent_peer_list_ans(): connect_socket_ans() failed.\n");
        return -2;
    }

    INFO_PRTF ("PUSHING 0x%08x PEER LIST TO %s:%d.\n", torrent->torrent_hash, peer->ip, peer->port);
    
    // Send message.
    if (write_bytes (peer_sock, msg, MSG_LEN) != MSG_LEN)
    {
        ERROR_PRTF ("ERROR push_torrent_peer_list_ans(): write_bytes() failed - %d\n", peer_sock);
        close (peer_sock);
        return -2;
    }

    // Send peer list.
    size_t buffer_size = (torrent->num_peers - peer_in_list) * PEER_LIST_MAX_BYTE_PER_PEER + 1;
    char *buffer = calloc (1, buffer_size);
    if (buffer == NULL)
    {
        ERROR_PRTF ("ERROR push_torrent_peer_list_ans(): calloc() failed. (ERRNO %d:%s)\n", errno, strerror(errno));
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
        ERROR_PRTF ("ERROR push_torrent_peer_list_ans(): write_bytes() failed - %d\n", peer_sock);
        free (buffer);
        close (peer_sock);
        return -2;
    }
    free (buffer);
    close (peer_sock);

    return 0;
}

// Push a torrent block status to a peer. 
// Returns 0 on success, -1 on timeout, -2 on error.
// Message protocol: 
// PUSH_TORRENT_BLOCK_STATUS [MY_ENGINE_HASH] [MY_LISTEN_PORT] [TORRENT_HASH] [BLOCK_STATUS]
// [BLOCK_STATUS] is a binary dump of the torrent's block status, which starts AFTER MSG_LEN bytes in the message.
int push_torrent_block_status_ans (peer_data_t *peer, torrent_t *torrent)
{
    if (peer == NULL || torrent == NULL)
    {
        ERROR_PRTF ("ERROR push_torrent_block_status_ans(): invalid argument.\n");
        return -2;
    }

    // Format message.
    char msg[MSG_LEN] = {0};
    sprintf (msg, "PUSH_TORRENT_BLOCK_STATUS 0x%08x %d 0x%08x", torrent->engine->engine_hash,
        torrent->engine->port, torrent->torrent_hash);

    // Connect to peer.
    int peer_sock = connect_socket_ans (peer->ip, peer->port);
    if (peer_sock == -1)
    {
        INFO_PRTF ("PUSHING 0x%08x BLOCK STATUS TO %s:%d - TIMEOUT.\n", torrent->torrent_hash, peer->ip, peer->port);
        return -1;
    }
    else if (peer_sock < -1)
    {
        ERROR_PRTF ("ERROR push_torrent_block_status_ans(): connect_socket_ans() failed.\n");
        return -2;
    }

    INFO_PRTF ("PUSHING 0x%08x BLOCK STATUS TO %s:%d.\n", torrent->torrent_hash, peer->ip, peer->port);

    // Send message.
    if (write_bytes (peer_sock, msg, MSG_LEN) != MSG_LEN)
    {
        ERROR_PRTF ("ERROR push_torrent_block_status_ans(): write_bytes() failed - %d\n", peer_sock);
        close (peer_sock);
        return -2;
    }

    // Send block status.
    if (write_bytes (peer_sock, torrent->block_status, torrent->num_blocks * sizeof (B_STAT)) != torrent->num_blocks * sizeof (B_STAT))
    {
        ERROR_PRTF ("ERROR push_torrent_block_status_ans(): write_bytes() failed - %d\n", peer_sock);
        close (peer_sock);
        return -2;
    }

    close (peer_sock);

    return 0;
} 

// Push block to a peer. 
// Returns 0 on success, -1 on timeout, -2 on error.
// Message protocol:
// PUSH_TORRENT_BLOCK [MY_ENGINE_HASH] [MY_LISTEN_PORT] [TORRENT_HASH] [BLOCK_INDEX] [BLOCK_DATA]
// [BLOCK_DATA] is a binary dump of the block data, which starts AFTER MSG_LEN bytes in the message.
int push_torrent_block_ans (peer_data_t *peer, torrent_t *torrent, size_t block_index)
{
    if (peer == NULL || torrent == NULL)
    {
        ERROR_PRTF ("ERROR push_torrent_block_ans(): invalid argument.\n");
        return -2;
    }
    if (block_index >= torrent->num_blocks)
    {
        ERROR_PRTF ("ERROR push_torrent_block_ans(): invalid block index.\n");
        return -2;
    }
    if (get_block_status (torrent, block_index) != B_DOWNLOADED)
    {
        ERROR_PRTF ("ERROR push_torrent_block_ans(): block not available.\n");
        return -2;
    }

    // Format message.
    char msg[MSG_LEN] = {0};
    sprintf (msg, "PUSH_TORRENT_BLOCK 0x%08x %d 0x%08x %ld", torrent->engine->engine_hash,
        torrent->engine->port, torrent->torrent_hash, block_index);
    
    // Connect to peer.
    int peer_sock = connect_socket_ans (peer->ip, peer->port);
    if (peer_sock == -1)
    {
        INFO_PRTF ("PUSHING 0x%08x BLOCK %ld TO %s:%d - TIMEOUT.\n", torrent->torrent_hash, block_index, peer->ip, peer->port);
        return -1;
    }
    else if (peer_sock < -1)
    {
        ERROR_PRTF ("ERROR push_torrent_block_ans(): connect_socket_ans() failed.\n");
        return -2;
    }

    INFO_PRTF ("PUSHING 0x%08x BLOCK %ld TO %s:%d.\n", torrent->torrent_hash, block_index, peer->ip, peer->port);

    // Send message.
    if (write_bytes (peer_sock, msg, MSG_LEN) != MSG_LEN)
    {
        ERROR_PRTF ("ERROR push_torrent_block_ans(): write_bytes() failed - %d\n", peer_sock);
        close (peer_sock);
        return -2;
    }

    // Send block data.
    if (write_bytes (peer_sock, get_block_ptr (torrent, block_index), BLOCK_SIZE) != BLOCK_SIZE)
    {
        ERROR_PRTF ("ERROR push_torrent_block_ans(): write_bytes() failed - %d\n", peer_sock);
        close (peer_sock);
        return -2;
    }


    return 0;
}

// Handle a request for a peer list, using push_torrent_peer_list_ans()
// Returns 0 on success, -1 on error.
// Message protocol: REQUEST_TORRENT_PEER_LIST [MY_ENGINE_HASH] [MY_LISTEN_PORT] [TORRENT_HASH]
int handle_request_torrent_peer_list_ans (torrent_engine_t *engine, int peer_sock, 
    peer_data_t *peer, torrent_t *torrent, char *msg_body)
{
    if (peer_sock < 0)
    {
        ERROR_PRTF ("ERROR handle_request_torrent_peer_list_ans(): invalid socket.\n");
        return -1;
    }
    if (engine == NULL || peer == NULL || torrent == NULL)
    {
        ERROR_PRTF ("ERROR handle_request_torrent_peer_list_ans(): invalid argument.\n");
        close (peer_sock);
        return -1;
    }
    close (peer_sock);

    // Check for extra arguments.
    char *val = strtok (msg_body, " ");
    if (val != NULL)
    {
        ERROR_PRTF ("ERROR handle_request_torrent_peer_list_ans(): invalid message.\n");
        return -1;
    }

    INFO_PRTF ("RECEIVED 0x%08x PEER LIST REQUEST FROM %s:%d.\n", torrent->torrent_hash, peer->ip, peer->port);

    // Push peer list.
    if (push_torrent_peer_list_ans (peer, torrent) < -1)
    {
        ERROR_PRTF ("ERROR handle_request_torrent_peer_list_ans(): push_torrent_peer_list_ans() failed.\n");
        return -1;
    }

    return 0;
}

// Handle a request for a block info, using push_torrent_block_status_ans()
// Returns 0 on success, -1 on error.
// Message protocol: REQUEST_TORRENT_BLOCK_STATUS [MY_ENGINE_HASH] [MY_LISTEN_PORT] [TORRENT_HASH]
int handle_request_torrent_block_status_ans (torrent_engine_t *engine, int peer_sock, 
    peer_data_t *peer, torrent_t *torrent, char *msg_body)
{
    if (peer_sock < 0)
    {
        ERROR_PRTF ("ERROR handle_request_torrent_block_status_ans(): invalid socket.\n");
        return -1;
    }
    if (engine == NULL || peer == NULL || torrent == NULL)
    {
        ERROR_PRTF ("ERROR handle_request_torrent_block_status_ans(): invalid argument.\n");
        close (peer_sock);
        return -1;
    }
    close (peer_sock);

    // Check for extra arguments.
    char *val = strtok (msg_body, " ");
    if (val != NULL)
    {
        ERROR_PRTF ("ERROR handle_request_torrent_block_status_ans(): invalid message.\n");
        return -1;
    }

    INFO_PRTF ("RECEIVED 0x%08x BLOCK STATUS REQUEST FROM %s:%d.\n", torrent->torrent_hash, peer->ip, peer->port);

    // Push block status.
    if (is_torrent_info_set (torrent) == 1 && push_torrent_block_status_ans (peer, torrent) < -1)
    {
        ERROR_PRTF ("ERROR handle_request_torrent_block_status_ans(): push_torrent_block_status_ans() failed.\n");
        return -1;
    }

    return 0;
}

// Handle a request for a block, using push_torrent_block_ans()
// Returns 0 on success, -1 on error.
// Message protocol: REQUEST_TORRENT_BLOCK [MY_ENGINE_HASH] [MY_LISTEN_PORT] [TORRENT_HASH] [BLOCK_INDEX]
int handle_request_torrent_block_ans (torrent_engine_t *engine, int peer_sock, 
    peer_data_t *peer, torrent_t *torrent, char *msg_body)
{
    if (peer_sock < 0)
    {
        ERROR_PRTF ("ERROR handle_request_torrent_block_ans(): invalid socket.\n");
        return -1;
    }
    if (engine == NULL || peer == NULL || torrent == NULL)
    {
        ERROR_PRTF ("ERROR handle_request_torrent_block_ans(): invalid argument.\n");
        return -1;
    }
    close (peer_sock);

    // Read block index.
    char *val = strtok (msg_body, " ");
    if (val == NULL)
    {
        ERROR_PRTF ("ERROR handle_request_torrent_block_ans(): invalid message.\n");
        return -1;
    }
    ssize_t block_index = atoi (val);
    if (is_torrent_info_set (torrent) == 1 && (block_index >= torrent->num_blocks || block_index < 0))
    {
        ERROR_PRTF ("ERROR handle_request_torrent_block_ans(): invalid block index %ld.\n", block_index);
        return -1;
    }

    // Check for extra arguments.
    val = strtok (NULL, " ");
    if (val != NULL)
    {
        ERROR_PRTF ("ERROR handle_request_torrent_block_ans(): invalid message.\n");
        return -1;
    }

    INFO_PRTF ("RECEIVED 0x%08x BLOCK %ld REQUEST FROM %s:%d.\n", torrent->torrent_hash, block_index, peer->ip, peer->port);
    
    // Push block.
    if (is_torrent_info_set (torrent) == 0 || get_block_status (torrent, block_index) != B_DOWNLOADED)
    {
        // ERROR_PRTF ("ERROR handle_request_torrent_block_ans(): block %ld is not completed.\n", block_index);
        return -1;
    }
    if (push_torrent_block_ans (peer, torrent, block_index) < -1)
    {
        ERROR_PRTF ("ERROR handle_request_torrent_block_ans(): push_torrent_block_ans() failed.\n");
        return -1;
    }

    return 0;
}

// Handle a push of a peer list.
// Returns 0 on success, -1 on error.
// PUSH_TORRENT_PEER_LIST [MY_ENGINE_HASH] [MY_LISTEN_PORT] [TORRENT_HASH] [NUM_PEERS] [PEER_0_IP]:[PEER_0_PORT] [PEE_1_IP]:[PEER_1_PORT] ...
// [PEER_X_IP]:[PEER_X_PORT] is a list of peers, which starts AFTER MSG_LEN bytes in the message.
// Each the list of peers is  [NUM_PEERS] * PEER_LIST_BYTE_PER_PEER bytes long, including the space.
int handle_push_torrent_peer_list_ans (torrent_engine_t *engine, int peer_sock, 
    peer_data_t *peer, torrent_t *torrent, char *msg_body)
{
    if (peer_sock < 0)
    {
        ERROR_PRTF ("ERROR handle_push_torrent_peer_list_ans(): invalid socket.\n");
        return -1;
    }
    if (engine == NULL || peer == NULL || torrent == NULL)
    {
        ERROR_PRTF ("ERROR handle_push_torrent_peer_list_ans(): invalid argument.\n");
        close (peer_sock);
        return -1;
    }

    // Read the number of peers.
    char *val = strtok (msg_body, " ");
    if (val == NULL)
    {
        ERROR_PRTF ("ERROR handle_push_torrent_peer_list_ans(): invalid message.\n");
        close (peer_sock);
        return -1;
    }
    ssize_t num_peers = atoi (val);
    if (num_peers <= 0)
    {
        close (peer_sock);
        return 0;
    }

    // Check for extra arguments.
    val = strtok (NULL, " ");
    if (val != NULL)
    {
        ERROR_PRTF ("ERROR handle_push_torrent_peer_list_ans(): invalid message.\n");
        close (peer_sock);
        return -1;
    }

    // Read peer list from the socket into buffer.
    size_t peer_list_buffer_size = num_peers * PEER_LIST_MAX_BYTE_PER_PEER;
    char *peer_list_buffer = calloc (1, peer_list_buffer_size);
    if (peer_list_buffer == NULL)
    {
        ERROR_PRTF ("ERROR handle_push_torrent_peer_list_ans(): calloc() failed. (ERRNO %d:%s)\n", errno, strerror(errno));;
        close (peer_sock);
        return -1;
    }
    if (read_bytes (peer_sock, peer_list_buffer, peer_list_buffer_size) != peer_list_buffer_size)
    {
        ERROR_PRTF ("ERROR handle_push_torrent_peer_list_ans(): read_bytes() failed.\n");
        close (peer_sock);
        free (peer_list_buffer);
        return -1;
    }
    close (peer_sock);

    INFO_PRTF ("RECEIVED 0x%08x PEER LIST FROM %s:%d.\n", torrent->torrent_hash, peer->ip, peer->port);

    // Parse peer list.
    char *buffer = peer_list_buffer;
    for (size_t i = 0; i < num_peers; i++)
    {
        val = strtok (buffer, " ");
        buffer = val + strlen(val) + 1;

        char *ip = strtok (val, ":");
        if (ip == NULL)
        {
            ERROR_PRTF ("ERROR handle_push_torrent_peer_list_ans(): invalid peer list.\n");
            free (peer_list_buffer);
            return -1;
        }
        char *port_str = strtok (NULL, "\0");
        if (port_str == NULL)
        {
            ERROR_PRTF ("ERROR handle_push_torrent_peer_list_ans(): invalid peer list.\n");
            free (peer_list_buffer);
            return -1;
        }
        int port = atoi (port_str);
        if (port <= 0 || port > 65535)
        {
            ERROR_PRTF ("ERROR handle_push_torrent_peer_list_ans(): invalid peer port.\n");
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
                ERROR_PRTF ("ERROR handle_push_torrent_peer_list_ans(): add_peer() failed.\n");
                free (peer_list_buffer);
                return -1;
            }
        }
    }

    return 0;
}

// Handle a push of a peer block status.
// Returns 0 on success, -1 on error.
// PUSH_TORRENT_BLOCK_STATUS [MY_ENGINE_HASH] [MY_LISTEN_PORT] [TORRENT_HASH] [BLOCK_STATUS]
// [BLOCK_STATUS] is a binary dump of the torrent's block status, which starts AFTER MSG_LEN bytes in the message.
int handle_push_torrent_block_status_ans (torrent_engine_t *engine, int peer_sock, 
    peer_data_t *peer, torrent_t *torrent, char *msg_body)
{
    if (peer_sock < 0)
    {
        ERROR_PRTF ("ERROR handle_push_torrent_block_status_ans(): invalid socket.\n");
        return -1;
    }
    if (engine == NULL || peer == NULL || torrent == NULL)
    {
        ERROR_PRTF ("ERROR handle_push_torrent_block_status_ans(): invalid argument.\n");
        close (peer_sock);
        return -1;
    }

    // Check for extra arguments.
    char *val = strtok (msg_body, " ");
    if (val != NULL)
    {
        ERROR_PRTF ("ERROR handle_push_torrent_block_status_ans(): invalid message.\n");
        close (peer_sock);
        return -1;
    }

    // Read block status from the socket into buffer.
    void *buffer = malloc (torrent->num_blocks * sizeof (B_STAT));
    if (buffer == NULL)
    {
        ERROR_PRTF ("ERROR handle_push_torrent_block_status_ans(): malloc() failed. (ERRNO %d:%s)\n", errno, strerror(errno));
        close (peer_sock);
        return -1;
    }
    if (read_bytes (peer_sock, buffer, torrent->num_blocks * sizeof (B_STAT)) 
        != torrent->num_blocks * sizeof (B_STAT))
    {
        ERROR_PRTF ("ERROR handle_push_torrent_block_status_ans(): read_bytes() failed.\n");
        close (peer_sock);
        return -1;
    }
    close (peer_sock);

    INFO_PRTF ("RECEIVED 0x%08x BLOCK STATUS FROM %s:%d.\n", torrent->torrent_hash, peer->ip, peer->port);

    // If torrent info is not set, return.
    if (is_torrent_info_set (torrent) == 0)
    {
        free (buffer);
        return -1;
    }

    // Copy buffer data.
    memcpy (peer->block_status, buffer, torrent->num_blocks * sizeof (B_STAT));

    free (buffer);

    return 0;
}

// Handle a push of a block.
// Check hash of the received block. If it does not match the expected hash, drop the block.
// Returns 0 on success, -1 on error.
// PUSH_TORRENT_BLOCK [MY_ENGINE_HASH] [MY_LISTEN_PORT] [TORRENT_HASH] [BLOCK_INDEX] [BLOCK_DATA]
// [BLOCK_DATA] is a binary dump of the block data, which starts AFTER MSG_LEN bytes in the message.
int handle_push_torrent_block_ans (torrent_engine_t *engine, int peer_sock, 
    peer_data_t *peer, torrent_t *torrent, char *msg_body)
{
    if (peer_sock < 0)
    {
        ERROR_PRTF ("ERROR handle_push_torrent_block_ans(): invalid socket.\n");
        return -1;
    }
    if (engine == NULL || peer == NULL || torrent == NULL)
    {
        ERROR_PRTF ("ERROR handle_push_torrent_block_ans(): invalid argument.\n");
        close (peer_sock);
        return -1;
    }

    // Read block index.
    char *val = strtok (msg_body, " ");
    if (val == NULL)
    {
        ERROR_PRTF ("ERROR handle_push_torrent_block_ans(): invalid message.\n");
        close (peer_sock);
        return -1;
    }
    ssize_t block_index = atoi (val);
    if (is_torrent_info_set (torrent) == 1 && (block_index >= torrent->num_blocks || block_index < 0))
    {
        ERROR_PRTF ("ERROR handle_push_torrent_block_ans(): invalid block index.\n");
        close (peer_sock);
        return -1;
    }

    // Check for extra arguments.
    val = strtok (NULL, " ");
    if (val != NULL)
    {
        ERROR_PRTF ("ERROR handle_push_torrent_block_ans(): invalid message.\n");
        close (peer_sock);
        return -1;
    }

    // Read block data to buffer
    void *buffer = malloc (BLOCK_SIZE);
    if (buffer == NULL)
    {
        ERROR_PRTF ("ERROR handle_push_torrent_block_ans(): malloc() failed. (ERRNO %d:%s)\n", errno, strerror(errno));
        close (peer_sock);
        return -1;
    }
    if (read_bytes (peer_sock, buffer, BLOCK_SIZE) != BLOCK_SIZE)
    {
        ERROR_PRTF ("ERROR handle_push_torrent_block_ans(): read_bytes() failed.\n");
        close (peer_sock);
        free (buffer);
        return -1;
    }
    close (peer_sock);

    INFO_PRTF ("RECEIVED 0x%08x BLOCK %ld FROM %s:%d.\n", torrent->torrent_hash, block_index, peer->ip, peer->port);

    // If torrent info is not set, return.
    if (is_torrent_info_set (torrent) == 0)
    {
        free (buffer);
        return -1;
    }

    // Check block hash.
    HASH_t block_hash = get_hash (buffer, BLOCK_SIZE);
    if (block_hash != torrent->block_hashes[block_index])
    {
        ERROR_PRTF ("ERROR handle_push_torrent_block_ans(): block hash does not match.\n");
        free (buffer);
        return -1;
    }

    // Copy buffer data to torrent.
    if (get_block_status (torrent, block_index) == B_MISSING
        || get_block_status (torrent, block_index) == B_REQUESTED)
    {
        memcpy (get_block_ptr (torrent, block_index), buffer, BLOCK_SIZE);
        torrent->block_status[block_index] = B_DOWNLOADED;
    }

    free (buffer);

    return 0;
}