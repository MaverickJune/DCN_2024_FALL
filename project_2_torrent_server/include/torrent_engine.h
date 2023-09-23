// NXC Data Communications torrent_engine.h for torrent
// Written by Jongseok Park (cakeng@snu.ac.kr)
// 2023. 9. 19


///// DO NOT MODIFY THIS FILE!! ////


#ifndef TORRENT_ENGINE_H
#define TORRENT_ENGINE_H

#include "torrent.h"

//// TORRENT ENGINE FUNCTIONS ////

// Client Routine.
int torrent_client (torrent_engine_t *engine);

// Server Routine.
int torrent_server (torrent_engine_t *engine);

// Open a socket and listen for incoming connections. 
// Returns the socket file descriptor on success, -1 on error.
int listen_socket (int port);

// Accept an incoming connection with a timeout. 
// Returns the connected socket file descriptor on success, -1 on error, -2 on timeout.
// MUST have a timeout of TIMEOUT_MSEC msec.
// HINT: Use poll(). See kbhit() in torrent_util.c for an example on using poll().
int accept_socket(int listen_sock, struct sockaddr_in *cli_addr, socklen_t *clilen);

// Connect to a server.
// Returns the socket file descriptor on success, -1 on error, -2 on timeout.
// MUST have a timeout of TIMEOUT_MSEC msec.
// HINT: Use poll(). See kbhit() in torrent_util.c for an example on using poll().
int connect_socket(char *server_ip, int port);

// Request torrent info from a remote host. Returns 0 on success, -1 on error.
// Message protocol: "REQUEST_TORRENT_INFO [MY_LISTEN_PORT] [TORRENT_HASH]"
int request_torrent_info (peer_data_t *peer, torrent_t *torrent);

// Push torrent INFO to peer. Returns 0 on success, -1 on error.
// Message protocol: "PUSH_TORRENT_INFO [MY_LISTEN_PORT] [TORRENT_HASH] [TORRENT_NAME] [FILE_SIZE]"
int push_torrent_info (peer_data_t *peer, torrent_t *torrent);

// Request peer's peer list from peer. Returns 0 on success, -1 on error.
int request_torrent_peers (peer_data_t *peer, torrent_t *torrent);

// Request peer's block info from peer. Returns 0 on success, -1 on error.
int request_torrent_block_status (peer_data_t *peer, torrent_t *torrent);

// Request block from peer. Returns 0 on success, -1 on error.
int request_torrent_block (peer_data_t *peer, torrent_t *torrent, size_t block_index);

// Push my list of peers to peer. Returns 0 on success, -1 on error.
// Be sure to remove the receiving peer from the peer list, if there is one.
int push_torrent_peers (peer_data_t *peer, torrent_t *torrent);

// Push torrent block status to peer. Returns 0 on success, -1 on error.
int push_torrent_block_status (peer_data_t *peer, torrent_t *torrent); 

// Push block to peer. Returns 0 on success, -1 on error.
int push_torrent_block (peer_data_t *peer, torrent_t *torrent, size_t block_index);

#endif // TORRENT_ENGINE_H