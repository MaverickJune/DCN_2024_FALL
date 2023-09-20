// NXC Data Communications Network socket_util.h
// Written by Jongseok Park (cakeng@snu.ac.kr)
// 2023. 9. 7

#ifndef SOCKET_UTIL_H
#define SOCKET_UTIL_H

#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "unistd.h"
#include "sys/socket.h"
#include "arpa/inet.h"
#include "netinet/tcp.h"

// Client side TCP socket initialization & connection function.
int client_init_and_connect_tcp_socket (char *server_ip, int server_port);
// Server side TCP socket initialization function.
int server_init_tcp_socket (int server_port);
// Accept incoming connections on the socket
int server_accept_tcp_socket (int socket);

#endif // SOCKET_UTIL_H