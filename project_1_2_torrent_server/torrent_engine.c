// NXC Data Communications torrent_engine.c for torrent
// Written by Jongseok Park (cakeng@snu.ac.kr)
// 2023. 9. 19

#include "torrent_functions.h"

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
    return 0;
}

// Accept an incoming connection with a timeout. 
// Returns the new socket file descriptor on success, -1 on error.
// Has a timeout of 50 msec.
int accept_socket(int sockfd, struct sockaddr_in *cli_addr, socklen_t *clilen)
{
    return 0;
}

// Connect to a remote host. 
// Returns the socket file descriptor on success, -1 on error.
// Has a timeout of 50 msec.
int connect_socket(char *host, int port)
{
    return 0;
}

// Request torrent info from a remote host. Returns 0 on success, -1 on error.
// Message protocol: "REQUEST_TORRENT_INFO [MY_LISTEN_PORT] [TORRENT_HASH]"
int request_torrent_info (peer_data_t *peer, torrent_t *torrent)
{
    return 0;
}

// Push torrent INFO to peer. Returns 0 on success, -1 on error.
// Message protocol: "PUSH_TORRENT_INFO [MY_LISTEN_PORT] [TORRENT_HASH] [TORRENT_NAME] [FILE_SIZE]"
int push_torrent_info (peer_data_t *peer, torrent_t *torrent)
{
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