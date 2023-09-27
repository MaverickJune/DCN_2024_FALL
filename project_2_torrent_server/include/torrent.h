// NXC Data Communications torrent.h for torrent
// Written by Jongseok Park (cakeng@snu.ac.kr)
// 2023. 9. 19


///// DO NOT MODIFY THIS FILE!! ////


#ifndef TORRENT_H
#define TORRENT_H

#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "stdint.h"
#include "signal.h"
#include "unistd.h"
#include "pthread.h"
#include "poll.h"
#include "sys/stat.h"
#include "sys/time.h"
#include "sys/socket.h"
#include "arpa/inet.h"
#include "netinet/tcp.h"

extern int print_info;

// Print macros
#define RED_PRTF(...) {printf("\033[0;31m"); printf(__VA_ARGS__); printf("\033[0m");}
#define GREEN_PRTF(...) {printf("\033[0;32m"); printf(__VA_ARGS__); printf("\033[0m");}
#define YELLOW_PRTF(...) {printf("\033[0;33m"); printf(__VA_ARGS__); printf("\033[0m");}
#define ERROR_PRTF(...) {fprintf(stderr, "\033[0;31m"); fprintf(stderr, "(%s:%d)", __FILE__, __LINE__); fprintf(stderr, __VA_ARGS__); fprintf(stderr, "\033[0m");}
#define INFO_PRTF(...) {if (print_info){printf("\033[0;34m"); printf(__VA_ARGS__); printf("\033[0m");}}

// Constants
#define STR_LEN 64
#define MSG_LEN (STR_LEN*4)
#define BLOCK_SIZE (32*1024) // 32KiB
#define DEFAULT_ARR_MAX_NUM 16
#define TIMEOUT_MSEC 10
#define HASH_SEED 0x12345678

// Hash of value 0 is used to indicate an invalid hash.
typedef uint32_t HASH_t;
typedef struct torrent torrent_t;
typedef struct torrent_engine torrent_engine_t;
typedef struct peer_data peer_data_t;

// Enum for block status.
typedef enum B_STAT
{
    B_ERROR = -1,
    B_MISSING = 0,
    B_REQUESTED = 1,
    B_READY = 2
} B_STAT;

// Struct for managing individual peers.
struct peer_data
{
    torrent_t *torrent;
    char ip[STR_LEN];
    int port;
    // Number of requests sent to the peer. Cleared when the peer sends a push message.
    size_t num_requests; 
    size_t last_torrent_info_request_msec;
    size_t last_peer_list_request_msec;
    size_t last_block_status_request_msec;
    size_t last_block_request_msec;
    B_STAT *block_status;

};

// Struct for managing individual torrents.
// The file data will be stored in memory.
// Therefore, the sum of all torrent file sizes should not exceed
// the available memory of the machine.
struct torrent
{
    torrent_engine_t *engine;
    HASH_t torrent_hash;

    char torrent_name[STR_LEN];
    size_t file_size;
    
    size_t num_blocks;
    HASH_t *block_hashes;
    B_STAT *block_status;

    size_t num_peers;
    size_t max_num_peers;
    peer_data_t **peers;

    size_t last_torrent_save_msec;
    size_t last_block_status_reset_msec;
    void* data;

};

// Struct for the main torrent execution engine.
struct torrent_engine
{
    int port;
    int listen_sock;
    HASH_t engine_hash;

    size_t num_torrents;
    size_t max_num_torrents;
    torrent_t **torrents;

    pthread_t thread;
    pthread_mutex_t mutex;
    int stop_engine;

};

//// TORRENT MANAGEMENT FUNCTIONS ////

// Initialize torrent. 
// Returns the torrent pointer if successful, NULL if error.
torrent_t *init_torrent (torrent_engine_t *engine);

// Create torrent from a file.
// Returns the torrent pointer if successful, NULL if error.
torrent_t *init_torrent_from_file (torrent_engine_t *engine, char *torrent_name, char* path);

// Create torrent from a hash. 
// Returns the torrent pointer if successful, NULL if error.
torrent_t *init_torrent_from_hash (torrent_engine_t *engine, HASH_t torrent_hash);

// Destroy torrent. 
void destroy_torrent (torrent_t *torrent);

// Set torrent info such as torrent name and file size.
// This function will allocate memory for the torrent data and set up block infos.
// Returns 0 on success, -1 on error.
int set_torrent_info (torrent_t *torrent, char *torrent_name, size_t file_size);

// Save torrent as file under SAVE_DIR. 
// Returns 0 on success, -1 on error.
int save_torrent_as_file (torrent_t *torrent);

// Find the index of torrent with torrent hash. 
// Returns the torrent index if found, -1 if not found.
ssize_t find_torrent (torrent_engine_t *engine, HASH_t torrent_hash);

// Find the index of torrent with torrent name.
// Returns the torrent index if found, -1 if not found.
ssize_t find_torrent_name (torrent_engine_t *engine, char* name);

// Get the status of a block with the given index.
// Returns the status if successful, B_ERROR if error.
B_STAT get_block_status (torrent_t *torrent, size_t block_index);

// Get the number of completed blocks.
// Returns the number of completed blocks if successful, -1 if error.
ssize_t get_num_completed_blocks (torrent_t *torrent);

// Find the index of first block that is missing, starting from start_idx.
// Returns the block index if found, -1 if not found, -2 if error.
ssize_t get_missing_block (torrent_t *torrent, size_t start_idx);

// Get the status of a peer block with the given index.
// Returns the status if successful, B_ERROR if error.
B_STAT get_peer_block_status (peer_data_t *peer, size_t block_index);

// Get the number of completed blocks from a peer.
// Returns the number of completed blocks if successful, -1 if error.
ssize_t get_peer_num_completed_blocks (peer_data_t *peer);

// Find pointer to the torrent block data indicated by the block index.
// Returns the pointer if successful, NULL if error.
void *get_block_ptr (torrent_t *torrent, size_t block_index);

// Check if the max number of torrents for the engine is reached.
// If so, double the max number of torrents for the engine.
// Must lock torrent mutex before calling this function.
int update_if_max_torrent_reached (torrent_engine_t *engine);

// Initialize peer data. 
// Returns the peer data pointer if successful, NULL if error.
peer_data_t *init_peer_data (torrent_t *torrent);

// Set peer block info such as initializing block data.
// Only works when set_torrent_info() has been called on the parent torrent.
// Returns 0 on success, -1 on error.
int set_peer_block_info (peer_data_t *peer_data);

// Add peer to torrent. 
// Returns the added peer index if successful, -1 if error.
ssize_t torrent_add_peer (torrent_t *torrent, char *ip, int port);

// Remove peer from torrent. Returns 0 on success, -1 on error.
int torrent_remove_peer (torrent_t *torrent, char *ip, int port);

// Destroy peer data.
void destroy_peer_data (peer_data_t *peer_data);

// Find peer in torrent with IP and port.
// Returns the peer index if found, -1 if not found.
ssize_t find_peer (torrent_t *torrent, char *ip, int port);

// Check if the max number of peers for the torrent is reached.
// If so, double the max number of peers for the torrent.
int update_if_max_peer_reached (torrent_t *torrent);

#endif // TORRENT_H