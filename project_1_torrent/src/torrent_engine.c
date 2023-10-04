// NXC Data Communications Network torrent_engine.c for BitTorrent-like P2P File Sharing System
// Written by Jongseok Park (cakeng@snu.ac.kr)
// 2023. 9. 19


#include "torrent_engine.h"
#include "torrent_ui.h"
#include "torrent_utils.h"

//// TORRENT ENGINE FUNCTIONS ////

// The implementations for below functions are provided as a reference for the project.

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

    // If torrent info exists, Push torrent info to peer.
    if (is_torrent_info_set (torrent) == 1 && push_torrent_info (peer, torrent) < -1)
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
    if (is_torrent_info_set (torrent) == 1)
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

    return 0;
}

//// TODO: IMPLEMENT THE FOLLOWING FUNCTIONS ////

// TODO: Complete the Client Routine.
int torrent_client (torrent_engine_t *engine)
{

    // Iterate through all torrents and take action.
    for (size_t i = 0; i < engine->num_torrents; i++)
    {
        torrent_t *torrent = engine->torrents[i];
        if (torrent == NULL)
        {
            ERROR_PRTF ("ERROR torrent_client(): invalid torrent.\n");
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
                    ERROR_PRTF ("ERROR torrent_client(): save_torrent_as_file() failed.\n");
                    return -1;
                }
            }

            // TODO: If RESET_BLOCK_STATUS_INTERVAL_MSEC has passed since last reset, reset blocks in
            //       REQUESTED state to MISSING state.

        }

        // Iterate through peers.
        for (size_t peer_idx = 0; peer_idx < torrent->num_peers; peer_idx++)
        {
            peer_data_t *peer = torrent->peers[peer_idx];
            // TODO: If torrent info has NOT been received, and REQUEST_TORRENT_INFO_INTERVAL_MSEC 
            //       has passed since last request, request the torrent info from the peer.
            // HINT: Make sure to use request_torrent_info_thread() instead of request_torrent_info().

            // TODO: If REQUEST_PEER_LIST_INTERVAL_MSEC has passed since last request, request peer list.
            // HINT: Make sure to use request_torrent_peer_list_thread() instead of request_torrent_peer_list().

            // TODO: If REQUEST_BLOCK_STATUS_INTERVAL_MSEC has passed since last request, request block status.
            // HINT: Make sure to use request_torrent_block_status_thread() instead of request_torrent_block_status().

            // TODO: If REQUEST_BLOCK_INTERVAL_MSEC has passed since last request, request a block.
            // HINT: Make sure to use request_torrent_block_thread() instead of request_torrent_block().
            //       Use get_rand_missing_block_that_peer_has () function to randomly select a block to request.
        }
    }

    return 0;
}

// TODO: Complete the server Routine.

int torrent_server (torrent_engine_t *engine)
{
    // TODO: Accept incoming connections.

    // TODO: Get peer ip and port.

    // TODO: Read message.

    // TODO: Parse command from message.
    
    // TODO: Parse peer engine hash from message.
    //       If the peer engine hash is the same as the local engine hash, ignore the message.

    // TODO: Parse peer listen port from message.

    // TODO: Parse peer torrent hash from message.

    // TODO: Add peer to torrent if it doesn't exist.

    // TODO: Call different handler function based on message command.
    // HINT: The handler functions take the engine, peer_sock, peer, torrent, and msg_body as arguments.
    //       The engine, peer, torrent arguments refers to the torrent engine, 
    //       peer that sent the message, and the torrent the message is about.
    //       The peer_sock argument refers to the socket that the message was received from. (Return value of accept_socket ())
    //       The msg_body argument refers to the part of the message after [TORRENT_HASH], excluding the space after [TORRENT_HASH], if it exists.
    
    return 0;
}

// TODO: Open a socket and listen for incoming connections. 
//       Returns the socket file descriptor on success, -1 on error.
int listen_socket (int port)
{
    return 0;
}

// TODO: Accept an incoming connection with a timeout. 
//       Return the connected socket file descriptor on success, -2 on error, -1 on timeout.
//       MUST use a non-blocking socket with a timeout of TIMEOUT_MSEC msec.
// HINT: Use poll() for timeout. kbhit() in torrent_util.c for an example on using poll().
int accept_socket(int listen_sock, struct sockaddr_in *cli_addr, socklen_t *clilen)
{
    return 0;
}

// TODO: Connect to a server with a timeout. 
//       Return the socket file descriptor on success, -2 on error, -1 on timeout.
//       MUST use a non-blocking socket with a timeout of TIMEOUT_MSEC msec.
// HINT: Use fcntl() to set the socket as non-blocking. 
//       Use poll() for timeout. See kbhit() in torrent_utils.c for an example on using poll().
int connect_socket(char *server_ip, int port)
{
    return 0;
}

// TODO: Request peer's peer list from a peer. 
//       Returns 0 on success, -1 on timeout, -2 on error.
//       Message protocol: REQUEST_TORRENT_PEER_LIST [MY_ENGINE_HASH] [MY_LISTEN_PORT] [TORRENT_HASH]
// HINT: 1.Update peer info. 2. Format message. 3. Connect to peer. 4. Send message.
//       Follow the example in request_torrent_info().
int request_torrent_peer_list (peer_data_t *peer, torrent_t *torrent)
{
    return 0;
}

// TODO: Request peer's block info from a peer. 
//       Returns 0 on success, -1 on timeout, -2 on error.
//       Message protocol: REQUEST_TORRENT_BLOCK_STATUS [MY_ENGINE_HASH] [MY_LISTEN_PORT] [TORRENT_HASH]
// HINT: 1.Update peer info. 2. Format message. 3. Connect to peer. 4. Send message.
//       Follow the example in request_torrent_info().
int request_torrent_block_status (peer_data_t *peer, torrent_t *torrent)
{
    return 0;
}

// TODO: Request a block from a peer. 
//       Returns 0 on success, -1 on timeout, -2 on error.
//       Message protocol: REQUEST_TORRENT_BLOCK [MY_ENGINE_HASH] [MY_LISTEN_PORT] [TORRENT_HASH] [BLOCK_INDEX]
// HINT: 1.Update peer info. 2. Format message. 3. Connect to peer. 4. Send message. 5. Set block status.
//       Follow the example in request_torrent_info().
int request_torrent_block (peer_data_t *peer, torrent_t *torrent, size_t block_index)
{
    return 0;
}

// TODO: Push my list of peers to a peer. 
//       Returns 0 on success, -1 on timeout, -2 on error.
//       Message protocol: 
//       PUSH_TORRENT_PEER_LIST [MY_ENGINE_HASH] [MY_LISTEN_PORT] [TORRENT_HASH] [NUM_PEERS] [PEER_0_IP]:[PEER_0_PORT] [PEER_1_IP]:[PEER_1_PORT] ...
//       [PEER_X_IP]:[PEER_X_PORT] is a list of peers, which starts AFTER MSG_LEN bytes in the message.
// HINT: 1. Format message. 2. Connect to peer. 3. Send message. 4. Send peer list.
//       [PEER_X_IP]:[PEER_X_PORT] is a list of peers, which starts AFTER MSG_LEN bytes in the message.
//       The peer list is  [NUM_PEERS] * PEER_LIST_MAX_BYTE_PER_PEER bytes long, including the space.
//       Make sure NOT to send the IP and port of the receiving peer back to itself.
//       Follow the example in push_torrent_info().
int push_torrent_peer_list (peer_data_t *peer, torrent_t *torrent)
{
    return 0;
}

// TODO: Push a torrent block status to a peer. 
//       Returns 0 on success, -1 on timeout, -2 on error.
//       Message protocol: 
//       PUSH_TORRENT_BLOCK_STATUS [MY_ENGINE_HASH] [MY_LISTEN_PORT] [TORRENT_HASH] [BLOCK_STATUS]
//       [BLOCK_STATUS] is a binary dump of the torrent's block status, which starts AFTER MSG_LEN bytes in the message.
// HINT: 1. Format message. 2. Connect to peer. 3. Send message. 4. Send block status.
//       Follow the example in push_torrent_info().
int push_torrent_block_status (peer_data_t *peer, torrent_t *torrent)
{
    return 0;
} 

// TODO: Push a block to a peer. 
//       Returns 0 on success, -1 on timeout, -2 on error.
//       Message protocol:
//       PUSH_TORRENT_BLOCK [MY_ENGINE_HASH] [MY_LISTEN_PORT] [TORRENT_HASH] [BLOCK_INDEX] [BLOCK_DATA]
//       [BLOCK_DATA] is a binary dump of the block data, which starts AFTER MSG_LEN bytes in the message.
// HINT: 1. Format message. 2. Connect to peer. 3. Send message. 4. Send block data.
//       Follow the example in push_torrent_info().
int push_torrent_block (peer_data_t *peer, torrent_t *torrent, size_t block_index)
{
    return 0;
}

// TODO: Handle a request for a peer list, using push_torrent_peer_list()
//       Returns 0 on success, -1 on error.
//       Message protocol: REQUEST_TORRENT_PEER_LIST [MY_ENGINE_HASH] [MY_LISTEN_PORT] [TORRENT_HASH]
// HINT: Follow the example in handle_request_torrent_info(). 
//       Use strtok(). Check if torrent info has been received.
int handle_request_torrent_peer_list (torrent_engine_t *engine, int peer_sock, 
    peer_data_t *peer, torrent_t *torrent, char *msg_body)
{
    return 0;
}

// TODO: Handle a request for a block info, using push_torrent_block_status()
//       Returns 0 on success, -1 on error.
//       Message protocol: REQUEST_TORRENT_BLOCK_STATUS [MY_ENGINE_HASH] [MY_LISTEN_PORT] [TORRENT_HASH]
// HINT: Follow the example in handle_request_torrent_info().
//       Use strtok(). Check if torrent info has been received.
int handle_request_torrent_block_status (torrent_engine_t *engine, int peer_sock, 
    peer_data_t *peer, torrent_t *torrent, char *msg_body)
{
    return 0;
}

// TODO: Handle a request for a block, using push_torrent_block()
//       Returns 0 on success, -1 on error.
//       Message protocol: REQUEST_TORRENT_BLOCK [MY_ENGINE_HASH] [MY_LISTEN_PORT] [TORRENT_HASH] [BLOCK_INDEX]
// HINT: Follow the example in handle_request_torrent_info().
//       Use strtok(). Check if torrent info has been received.
int handle_request_torrent_block (torrent_engine_t *engine, int peer_sock, 
    peer_data_t *peer, torrent_t *torrent, char *msg_body)
{
    return 0;
}

// TODO: Handle a push of a peer list.
//       Returns 0 on success, -1 on error.
//       PUSH_TORRENT_PEER_LIST [MY_ENGINE_HASH] [MY_LISTEN_PORT] [TORRENT_HASH] [NUM_PEERS] [PEER_0_IP]:[PEER_0_PORT] [PEE_1_IP]:[PEER_1_PORT] ...
//       [PEER_X_IP]:[PEER_X_PORT] is a list of peers, which starts AFTER MSG_LEN bytes in the message.
// HINT: Follow the example in handle_push_torrent_info().
//       Use strtok(). Check if torrent info has been received.
int handle_push_torrent_peer_list (torrent_engine_t *engine, int peer_sock, 
    peer_data_t *peer, torrent_t *torrent, char *msg_body)
{
    return 0;
}

// TODO: Handle a push of a peer block status.
//       Returns 0 on success, -1 on error.
//       PUSH_TORRENT_BLOCK_STATUS [MY_ENGINE_HASH] [MY_LISTEN_PORT] [TORRENT_HASH] [BLOCK_STATUS]
//       [BLOCK_STATUS] is a binary dump of the torrent's block status, which starts AFTER MSG_LEN bytes in the message.
// HINT: Follow the example in handle_push_torrent_info().
//       Use strtok(). Check if torrent info has been received.
int handle_push_torrent_block_status (torrent_engine_t *engine, int peer_sock, 
    peer_data_t *peer, torrent_t *torrent, char *msg_body)
{
    return 0;
}

// TODO: Handle a push of a block.
//       Check hash of the received block. If it does NOT match the expected hash, drop the received block.
//       Returns 0 on success, -1 on error.
//       PUSH_TORRENT_BLOCK [MY_ENGINE_HASH] [MY_LISTEN_PORT] [TORRENT_HASH] [BLOCK_INDEX] [BLOCK_DATA]
//       [BLOCK_DATA] is a binary dump of the block data, which starts AFTER MSG_LEN bytes in the message.
// HINT: Follow the example in handle_push_torrent_info().
//       Use strtok(). Check if torrent info has been received.
int handle_push_torrent_block (torrent_engine_t *engine, int peer_sock, 
    peer_data_t *peer, torrent_t *torrent, char *msg_body)
{
    return 0;
}
