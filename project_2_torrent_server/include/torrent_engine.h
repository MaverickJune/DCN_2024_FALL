// NXC Data Communications torrent_engine.h for torrent
// Written by Jongseok Park (cakeng@snu.ac.kr)
// 2023. 9. 19


///// DO NOT MODIFY THIS FILE!! ////


#ifndef TORRENT_ENGINE_H
#define TORRENT_ENGINE_H

#include "torrent.h"

//// TORRENT ENGINE FUNCTIONS ////

// The implementations for below functions are provided as a reference for the project.

// Request torrent info from a remote host. Returns 0 on success, -1 on error.
// Message protocol: "REQUEST_TORRENT_INFO [MY_ENGINE_HASH] [MY_LISTEN_PORT] [TORRENT_HASH]"
int request_torrent_info (peer_data_t *peer, torrent_t *torrent);

// Push torrent INFO to peer. Returns 0 on success, -1 on error.
// Message protocol: 
// "PUSH_TORRENT_INFO [MY_ENGINE_HASH] [MY_LISTEN_PORT] [TORRENT_HASH] [TORRENT_NAME] [FILE_SIZE]" [BLOCK_HASH]
// [BLOCK_HASH] is a binary dump of the torrent's block hashes.
int push_torrent_info (peer_data_t *peer, torrent_t *torrent);

// Handle a request for torrent info, using push_torrent_info()
// Returns 0 on success, -1 on error.
int handle_request_torrent_info (torrent_engine_t *engine, int peer_sock, peer_data_t *peer, torrent_t *torrent, char *msg_body);

// Handle a push of torrent info.
// Returns 0 on success, -1 on error.
int handle_push_torrent_info (torrent_engine_t *engine, int peer_sock, peer_data_t *peer, torrent_t *torrent, char *msg_body);


//// TODO: IMPLEMENT THE FOLLOWING FUNCTIONS IN torrent_engine.c ////

// Client Routine.
// Returns 0 on success, -1 on error.
int torrent_client (torrent_engine_t *engine);

// Server Routine.
// Returns 0 on success, -1 on error.
int torrent_server (torrent_engine_t *engine);

// Open a socket and listen for incoming connections. 
// Returns the socket file descriptor on success, -1 on error.
int listen_socket (int port);

// Accept an incoming connection with a timeout. 
// Returns the connected socket file descriptor on success, -2 on error, -1 on timeout.
// MUST have a timeout of TIMEOUT_MSEC msec.
// HINT: Use poll(). See kbhit() in torrent_utils.c for an example on using poll().
int accept_socket(int listen_sock, struct sockaddr_in *cli_addr, socklen_t *clilen);

// Connect to a server.
// Returns the socket file descriptor on success, -2 on error, -1 on timeout.
// MUST have a timeout of TIMEOUT_MSEC msec.
// HINT: Use poll(). See kbhit() in torrent_utils.c for an example on using poll().
int connect_socket(char *server_ip, int port);

// Request peer's peer list from peer. 
// Returns 0 on success, -1 on timeout, -2 on error.
int request_torrent_peer_list (peer_data_t *peer, torrent_t *torrent);

// Request peer's block info from peer. 
// Returns 0 on success, -1 on timeout, -2 on error.
int request_torrent_block_status (peer_data_t *peer, torrent_t *torrent);

// Request block from peer. 
// Returns 0 on success, -1 on timeout, -2 on error.
int request_torrent_block (peer_data_t *peer, torrent_t *torrent, size_t block_index);

// Push my list of peers to peer. 
// Returns 0 on success, -1 on timeout, -2 on error.
// Be sure to remove the receiving peer from the peer list, if there is one.
int push_torrent_peer_list (peer_data_t *peer, torrent_t *torrent);

// Push torrent block status to peer. 
// Returns 0 on success, -1 on timeout, -2 on error.
int push_torrent_block_status (peer_data_t *peer, torrent_t *torrent); 

// Push block to peer. 
// Returns 0 on success, -1 on timeout, -2 on error.
int push_torrent_block (peer_data_t *peer, torrent_t *torrent, size_t block_index);

// Handle a request for peer list, using push_torrent_peer_list()
// Returns 0 on success, -1 on error.
int handle_request_torrent_peer_list (torrent_engine_t *engine, int peer_sock, peer_data_t *peer, torrent_t *torrent, char *msg_body);

// Handle a request for block info, using push_torrent_block_status()
// Returns 0 on success, -1 on error.
int handle_request_torrent_block_status (torrent_engine_t *engine, int peer_sock, peer_data_t *peer, torrent_t *torrent, char *msg_body);

// Handle a request for block, using push_torrent_block()
// Returns 0 on success, -1 on error.
int handle_request_torrent_block (torrent_engine_t *engine, int peer_sock, peer_data_t *peer, torrent_t *torrent, char *msg_body);

// Handle a push of peer list.
// Returns 0 on success, -1 on error.
int handle_push_torrent_peer_list (torrent_engine_t *engine, int peer_sock, peer_data_t *peer, torrent_t *torrent, char *msg_body);

// Handle a push of block status.
// Returns 0 on success, -1 on error.
int handle_push_torrent_block_status (torrent_engine_t *engine, int peer_sock, peer_data_t *peer, torrent_t *torrent, char *msg_body);

// Handle a push of block.
// Returns 0 on success, -1 on error.
int handle_push_torrent_block (torrent_engine_t *engine, int peer_sock, peer_data_t *peer, torrent_t *torrent, char *msg_body);

#endif // TORRENT_ENGINE_H