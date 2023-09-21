// NXC Data Communications torrent_functions.h for torrent
// Written by Jongseok Park (cakeng@snu.ac.kr)
// 2023. 9. 19


///// DO NOT MODIFY THIS FILE!! ////


#ifndef TORRENT_FUNCTIONS_H
#define TORRENT_FUNCTIONS_H

#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "unistd.h"
#include "signal.h"
#include "stdint.h"
#include "pthread.h"
#include "stdatomic.h"
#include "sys/time.h"
#include "sys/socket.h"
#include "arpa/inet.h"
#include "netinet/tcp.h"
#include "pthread.h"

#define update() printf("\033[H\033[J")
#define gotoxy(x, y) printf("\033[%d;%dH", x, y)
#define red() printf("\033[0;31m")
#define green() printf("\033[0;32m")
#define yellow() printf("\033[0;33m")
#define white() printf("\033[0;37m")
#define reset() printf("\033[0m")

#define ERROR_PRT(...) {fprintf(stderr, "\033[0;31m"); fprintf(stderr, __VA_ARGS__); fprintf(stderr, "\033[0m");}

#define STR_LEN 128
#define DEFAULT_MAX_NUM 16
#define BLOCK_SIZE (32*1024) // 32KiB
#define HASH_SEED 0x12345678
#define TIMEOUT_MSEC 30
#define RAND_WAIT_RANGE 30
#define MAX_QUEUED_CONNECTIONS 16
#define PRINT_COL_NUM 10
#define SAVE_DIR "./torrents/"


// Hash of value 0 is used to indicate an invalid hash.
typedef uint32_t HASH_t;
typedef struct torrent torrent_t;
typedef struct torrent_engine torrent_engine_t;
typedef struct peer_data peer_data_t;


// Struct for managing individual peers.
struct peer_data
{
    torrent_t *torrent;
    char ip[STR_LEN];
    int port;
    size_t num_requests;
    uint8_t *block_status;

};

// Struct for managing individual torrents.
// The file data will be stored in memory.
// Therefore, the sum of all torrent file sizes should not exceed
// the available memory of the machine.
struct torrent
{
    HASH_t torrent_hash;

    char torrent_name[STR_LEN];
    size_t file_size;
    
    size_t num_blocks;
    HASH_t *block_hashes;
    char *block_status;

    size_t num_peers;
    size_t max_num_peers;
    peer_data_t **peers;

    void* data;

};

// Struct for the main torrent execution engine.
struct torrent_engine
{
    int port;
    HASH_t engine_hash;

    size_t num_torrents;
    size_t max_num_torrents;
    torrent_t **torrents;

    pthread_t thread;
    pthread_mutex_t mutex;
    int stop_engine;

};

//// PROBLEM FUNCTIONS ////

// Client Routine.
int torrent_client (torrent_engine_t *engine);

// Server Routine.
int torrent_server (torrent_engine_t *engine);

// Open a socket and listen for incoming connections. 
// Returns the socket file descriptor on success, -1 on error.
int listen_socket (int port);

// Accept an incoming connection with a timeout. 
// Returns the new socket file descriptor on success, -1 on error.
// Has a timeout of 50 msec.
int accept_socket(int sockfd, struct sockaddr_in *cli_addr, socklen_t *clilen);

// Connect to a remote host. 
// Returns the socket file descriptor on success, -1 on error.
// Has a timeout of 50 msec.
int connect_socket(char *host, int port);

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

//// UI ELEMENTS ////  

// Wrapper functions for the client and server threads.
void *torrent_engine_thread (void *_engine);

// Initialize torrent engine. Returns the engine pointer on success, NULL on error.
torrent_engine_t *init_torrent_engine (int port);

// Destroy torrent engine.
void destroy_torrent_engine (torrent_engine_t *engine);

// Create new torrent from a file. Returns the torrent hash on success, 0 on error.
HASH_t create_new_torrent (torrent_engine_t *engine, char *torrent_name, char *path);

// Add torrent to engine, using a hash. Returns 0 on success, -1 on error.
int add_torrent (torrent_engine_t *engine, HASH_t torrent_hash);

// Remove torrent from engine, using a hash. Returns 0 on success, -1 on error.
// Returns 0 when the torrent is not found.
int remove_torrent (torrent_engine_t *engine, HASH_t torrent_hash);

// Add peer to torrent with the given hash. Returns 0 on success, -1 on error.
int add_peer (torrent_engine_t *engine, HASH_t torrent_hash, char *ip, int port);

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

//// TORRENT MANAGEMENT FUNCTIONS ////

// Initialize torrent. 
// Returns the torrent pointer if successful, NULL if error.
torrent_t *init_torrent ();

// Create torrent from a file.
// Returns the torrent pointer if successful, NULL if error.
torrent_t *init_torrent_from_file (char *torrent_name, char* path);

// Create torrent from a hash. 
// Returns the torrent pointer if successful, NULL if error.
torrent_t *init_torrent_from_hash (HASH_t torrent_hash);

// Destroy torrent. 
void destroy_torrent (torrent_t *torrent);

// Set torrent info such as torrent name and file size.
// This function will allocate memory for the torrent data and set up block infos.
// Returns 0 on success, -1 on error.
int set_torrent_info (torrent_t *torrent, char *torrent_name, size_t file_size);

// Save torrent as file under SAVE_DIR. Returns 0 on success, -1 on error.
int save_torrent_as_file (torrent_t *torrent);

// Find the index of torrent with torrent hash. 
// Returns the torrent index if found, -1 if not found.
ssize_t find_torrent (torrent_engine_t *engine, HASH_t torrent_hash);

// Find the index of torrent with torrent name.
// Returns the torrent index if found, -1 if not found.
ssize_t find_torrent_name (torrent_engine_t *engine, char* name);

// Get the number of completed blocks.
// Returns the number of completed blocks if successful, -1 if error.
ssize_t get_num_completed_blocks (torrent_t *torrent);

// Find the index of first block that is missing after start_idx.
// Returns the block index if found, -1 if not found.
ssize_t get_missing_block (torrent_t *torrent, size_t start_idx);

// Get the status of a peer block with the given index.
char get_peer_block_status (peer_data_t *peer, size_t block_index);

// Find pointer to the torrent block data indicated by the block index.
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

// Add peer to torrent. Returns 0 on success, -1 on error.
int torrent_add_peer (torrent_t *torrent, char *ip, int port);

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

//// UTILITY FUNCTIONS ////

// Get hash value of data.
// Dependent on the endianess of the machine.
// Assumes little endian for this project.
HASH_t get_hash (void* data, size_t len);

// Convert string to hash value.
// Only accepts string of "0x[0-9a-fA-F]{8}" format.
// Returns the hash value on success, 0 on error.
HASH_t str_to_hash (char *str);

// Check if ip is in IPv4.
// Only accepts xxx.xxx.xxx.xxx format.
// Returns the length of string on success, -1 on error.
int check_ipv4 (char *ip);

// Get time in milliseconds.
size_t get_time_msec();

// Get the size of string from an integer.
int get_int_str_len (size_t num);

// Read a file into memory.
// Returns the number of bytes read on success, -1 on error.
ssize_t read_file (char *path, void *data);

// Get size of a file.
// Returns -1 on error.
ssize_t get_file_size (char *path);

// Read size bytes from socket to buffer.
ssize_t read_bytes (int socket, char *buffer, size_t size);

// Write size bytes from buffer to socket.
ssize_t write_bytes (int socket, char *buffer, size_t size);

// Checks if there is a key press in stdin, with timeout.
// Returns 1 if there is a key press, 0 if else.
// From https://stackoverflow.com/questions/3711830/set-a-timeout-for-reading-stdin
int kbhit();

#endif // TORRENT_FUNCTIONS_H