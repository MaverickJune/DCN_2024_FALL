// NXC Data Communications torrent_utils.h for torrent
// Written by Jongseok Park (cakeng@snu.ac.kr)
// 2023. 9. 19


///// DO NOT MODIFY THIS FILE!! ////


#ifndef TORRENT_UTILS_H
#define TORRENT_UTILS_H

#include "torrent.h"

//// UTILITY UTILS ////

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

// Get elapsed time in milliseconds.
size_t get_elapsed_msec();

// Get the size of string from an integer.
int get_int_str_len (size_t num);

// Read a file into memory.
// Returns the number of bytes read on success, -1 on error.
ssize_t read_file (char *path, void *data);

// Get size of a file.
// Returns -1 on error.
ssize_t get_file_size (char *path);

// Read size bytes from socket to buffer.
// Returns the number of bytes read on success, -1 on error.
ssize_t read_bytes (int socket, void *buffer, size_t size);

// Write size bytes from buffer to socket.
// Returns the number of bytes written on success, -1 on error.
ssize_t write_bytes (int socket, void *buffer, size_t size);

// Checks if there is a key press in stdin, with timeout.
// Returns 1 if there is a key press, 0 if else.
int kbhit();

#endif // TORRENT_UTILS_H