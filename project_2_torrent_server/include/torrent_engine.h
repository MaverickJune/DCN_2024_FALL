// NXC Data Communications torrent_engine.h for torrent
// Written by Jongseok Park (cakeng@snu.ac.kr)
// 2023. 9. 19


///// DO NOT MODIFY THIS FILE!! ////


#ifndef TORRENT_ENGINE_H
#define TORRENT_ENGINE_H

#include "torrent.h"

// Constants
#define REQUEST_TORRENT_INFO_INTERVAL_MSEC 1000
#define REQUEST_PEER_LIST_INTERVAL_MSEC 3000
#define REQUEST_BLOCK_STATUS_INTERVAL_MSEC 1500
#define REQUEST_BLOCK_INTERVAL_MSEC 150
#define RESET_BLOCK_STATUS_INTERVAL_MSEC 5000
#define TORRENT_SAVE_INTERVAL_MSEC 5000
#define PEER_LIST_MAX_BYTE_PER_PEER 21 // [PEER_X_IP]:[PEER_X_PORT] = "xxx.xxx.xxx.xxx:xxxxx "
#define MAX_QUEUED_CONNECTIONS 16

//// TORRENT ENGINE FUNCTIONS ////

// The implementations for below functions are provided as a reference for the project.

// Request torrent info from a remote host. Returns 0 on success, -1 on timeout, -2 on error.
// Message protocol: REQUEST_TORRENT_INFO [MY_ENGINE_HASH] [MY_LISTEN_PORT] [TORRENT_HASH]
int request_torrent_info (peer_data_t *peer, torrent_t *torrent);

// Push torrent INFO to peer. Returns 0 on success, -1 on timeout, -2 on error.
// Message protocol: 
// PUSH_TORRENT_INFO [MY_ENGINE_HASH] [MY_LISTEN_PORT] [TORRENT_HASH] [TORRENT_NAME] [FILE_SIZE] [BLOCK_HASH]
// [BLOCK_HASH] is a binary dump of the torrent's block hashes, which starts AFTER MSG_LEN bytes in the message.
int push_torrent_info (peer_data_t *peer, torrent_t *torrent);

// Handle a request for torrent info, using push_torrent_info()
// Returns 0 on success, -1 on error.
// Message protocol: REQUEST_TORRENT_INFO [MY_ENGINE_HASH] [MY_LISTEN_PORT] [TORRENT_HASH]
int handle_request_torrent_info (torrent_engine_t *engine, int peer_sock, 
    peer_data_t *peer, torrent_t *torrent, char *msg_body);

// Handle a push of torrent info.
// Returns 0 on success, -1 on error.
// Message protocol: 
// PUSH_TORRENT_INFO [MY_ENGINE_HASH] [MY_LISTEN_PORT] [TORRENT_HASH] [TORRENT_NAME] [FILE_SIZE] [BLOCK_HASH]
// [BLOCK_HASH] is a binary dump of the torrent's block hashes, which starts AFTER MSG_LEN bytes in the message.
int handle_push_torrent_info (torrent_engine_t *engine, int peer_sock, 
    peer_data_t *peer, torrent_t *torrent, char *msg_body);

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
// HINT: Use poll(). See kbhit() in torrent_util.c for an example on using poll().
int accept_socket(int listen_sock, struct sockaddr_in *cli_addr, socklen_t *clilen);

// Connect to a server.
// Returns the socket file descriptor on success, -2 on error, -1 on timeout.
// MUST have a timeout of TIMEOUT_MSEC msec.
// HINT: Use poll(). See kbhit() in torrent_utils.c for an example on using poll().
int connect_socket(char *server_ip, int port);

// Request peer's peer list from a peer. 
// Returns 0 on success, -1 on timeout, -2 on error.
// Message protocol: REQUEST_TORRENT_PEER_LIST [MY_ENGINE_HASH] [MY_LISTEN_PORT] [TORRENT_HASH]
int request_torrent_peer_list (peer_data_t *peer, torrent_t *torrent);

// Request peer's block info from a peer. 
// Returns 0 on success, -1 on timeout, -2 on error.
// Message protocol: REQUEST_TORRENT_BLOCK_STATUS [MY_ENGINE_HASH] [MY_LISTEN_PORT] [TORRENT_HASH]
int request_torrent_block_status (peer_data_t *peer, torrent_t *torrent);

// Request a block from a peer. 
// Returns 0 on success, -1 on timeout, -2 on error.
// Message protocol: REQUEST_TORRENT_BLOCK [MY_ENGINE_HASH] [MY_LISTEN_PORT] [TORRENT_HASH] [BLOCK_INDEX]
int request_torrent_block (peer_data_t *peer, torrent_t *torrent, size_t block_index);

// Push my list of peers to a peer. 
// Returns 0 on success, -1 on timeout, -2 on error.
// Message protocol: 
// PUSH_TORRENT_PEER_LIST [MY_ENGINE_HASH] [MY_LISTEN_PORT] [TORRENT_HASH] [NUM_PEERS] [PEER_0_IP]:[PEER_0_PORT] [PEE_1_IP]:[PEER_1_PORT] ...
// [PEER_X_IP]:[PEER_X_PORT] is a list of peers, which starts AFTER MSG_LEN bytes in the message.
// Each the list of peers is  [NUM_PEERS] * PEER_LIST_BYTE_PER_PEER bytes long, including the space.
// Make sure not to send the IP and port of the receiving peer back to itself.
int push_torrent_peer_list (peer_data_t *peer, torrent_t *torrent);

// Push a torrent block status to a peer. 
// Returns 0 on success, -1 on timeout, -2 on error.
// Message protocol: 
// PUSH_TORRENT_BLOCK_STATUS [MY_ENGINE_HASH] [MY_LISTEN_PORT] [TORRENT_HASH] [BLOCK_STATUS]
// [BLOCK_STATUS] is a binary dump of the torrent's block status, which starts AFTER MSG_LEN bytes in the message.
int push_torrent_block_status (peer_data_t *peer, torrent_t *torrent); 

// Push block to a peer. 
// Returns 0 on success, -1 on timeout, -2 on error.
// Message protocol:
// PUSH_TORRENT_BLOCK [MY_ENGINE_HASH] [MY_LISTEN_PORT] [TORRENT_HASH] [BLOCK_INDEX] [BLOCK_DATA]
// [BLOCK_DATA] is a binary dump of the block data, which starts AFTER MSG_LEN bytes in the message.
int push_torrent_block (peer_data_t *peer, torrent_t *torrent, size_t block_index);

// Handle a request for a peer list, using push_torrent_peer_list()
// Returns 0 on success, -1 on error.
// Message protocol: REQUEST_TORRENT_PEER_LIST [MY_ENGINE_HASH] [MY_LISTEN_PORT] [TORRENT_HASH]
int handle_request_torrent_peer_list (torrent_engine_t *engine, int peer_sock, 
    peer_data_t *peer, torrent_t *torrent, char *msg_body);

// Handle a request for a block info, using push_torrent_block_status()
// Returns 0 on success, -1 on error.
// Message protocol: REQUEST_TORRENT_BLOCK_STATUS [MY_ENGINE_HASH] [MY_LISTEN_PORT] [TORRENT_HASH]
int handle_request_torrent_block_status (torrent_engine_t *engine, int peer_sock, 
    peer_data_t *peer, torrent_t *torrent, char *msg_body);

// Handle a request for a block, using push_torrent_block()
// Returns 0 on success, -1 on error.
// Message protocol: REQUEST_TORRENT_BLOCK [MY_ENGINE_HASH] [MY_LISTEN_PORT] [TORRENT_HASH] [BLOCK_INDEX]
int handle_request_torrent_block (torrent_engine_t *engine, int peer_sock, 
    peer_data_t *peer, torrent_t *torrent, char *msg_body);

// Handle a push of a peer list.
// Returns 0 on success, -1 on error.
// PUSH_TORRENT_PEER_LIST [MY_ENGINE_HASH] [MY_LISTEN_PORT] [TORRENT_HASH] [NUM_PEERS] [PEER_0_IP]:[PEER_0_PORT] [PEE_1_IP]:[PEER_1_PORT] ...
// [PEER_X_IP]:[PEER_X_PORT] is a list of peers, which starts AFTER MSG_LEN bytes in the message.
// Each the list of peers is  [NUM_PEERS] * PEER_LIST_BYTE_PER_PEER bytes long, including the space.
int handle_push_torrent_peer_list (torrent_engine_t *engine, int peer_sock, 
    peer_data_t *peer, torrent_t *torrent, char *msg_body);

// Handle a push of a peer block status.
// Returns 0 on success, -1 on error.
// PUSH_TORRENT_BLOCK_STATUS [MY_ENGINE_HASH] [MY_LISTEN_PORT] [TORRENT_HASH] [BLOCK_STATUS]
// [BLOCK_STATUS] is a binary dump of the torrent's block status, which starts AFTER MSG_LEN bytes in the message.
int handle_push_torrent_block_status (torrent_engine_t *engine, int peer_sock, 
    peer_data_t *peer, torrent_t *torrent, char *msg_body);

// Handle a push of a block.
// Check hash of the received block. If it does not match the expected hash, drop the block.
// Returns 0 on success, -1 on error.
// PUSH_TORRENT_BLOCK [MY_ENGINE_HASH] [MY_LISTEN_PORT] [TORRENT_HASH] [BLOCK_INDEX] [BLOCK_DATA]
// [BLOCK_DATA] is a binary dump of the block data, which starts AFTER MSG_LEN bytes in the message.
int handle_push_torrent_block (torrent_engine_t *engine, int peer_sock, 
    peer_data_t *peer, torrent_t *torrent, char *msg_body);

#endif // TORRENT_ENGINE_H