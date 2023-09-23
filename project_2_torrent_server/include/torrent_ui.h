// NXC Data Communications torrent_ui.h for torrent
// Written by Jongseok Park (cakeng@snu.ac.kr)
// 2023. 9. 19


///// DO NOT MODIFY THIS FILE!! ////


#ifndef TORRENT_UI_H
#define TORRENT_UI_H

#include "torrent.h"

//// UI FUNCTIONS ////  

// Wrapper functions for the client and server threads.
void *torrent_engine_thread (void *_engine);

// Initialize torrent engine. Returns the engine pointer on success, NULL on error.
torrent_engine_t *init_torrent_engine (int port);

// Destroy torrent engine.
void destroy_torrent_engine (torrent_engine_t *engine);

// Create new torrent from a file. Returns the torrent hash on success, 0 on error.
HASH_t create_new_torrent (torrent_engine_t *engine, char *torrent_name, char *path);

// Add torrent to engine, using a hash. 
// Returns the added torrent index on success, -1 on error.
ssize_t add_torrent (torrent_engine_t *engine, HASH_t torrent_hash);

// Remove torrent from engine, using a hash. Returns 0 on success, -1 on error.
// Returns 0 when the torrent is not found.
int remove_torrent (torrent_engine_t *engine, HASH_t torrent_hash);

// Add peer to torrent with the given hash. 
// Returns the added peer index on success, -1 on error.
ssize_t add_peer (torrent_engine_t *engine, HASH_t torrent_hash, char *ip, int port);

// Remove peer from torrent with the given hash. Returns 0 on success, -1 on error.
// Returns 0 when the peer is not found.
int remove_peer (torrent_engine_t *engine, HASH_t torrent_hash, char *ip, int port);

// Print the status of the engine.
void print_engine_status (torrent_engine_t *engine);

// Print the status of a torrent.
void print_torrent_status (torrent_t *torrent);

// Print the status of a torrent with hash.
void print_torrent_status_hash (torrent_engine_t *engine, HASH_t torrent_hash);

// Print the status of a peer.
void print_peer_status (peer_data_t *peer);

#endif // TORRENT_UI_H