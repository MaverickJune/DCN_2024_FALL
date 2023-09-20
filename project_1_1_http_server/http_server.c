// NXC Data Communications main.c for HTTP server
// Written by Jongseok Park (cakeng@snu.ac.kr)
// 2023. 9. 11


///// DO NOT MODIFY THIS FILE!! ////

#include "http_functions.h"

int server_engine (int server_port);
int server_routine (int client_sock);

int server_engine_ans (int server_port);
int server_routine_ans (int client_sock);

int main (int argc, char **argv)
{
    // Parse inputs
    if (argc != 2) 
    {
        printf ("Usage: %s <port>\n", argv[0]);
        printf ("ex) %s 62123\n", argv[0]);
        return 1;
    }
    int server_port = atoi(argv[1]);
    if (server_port <= 0 || server_port > 65535)
    {
        printf ("Invalid port number: %s\n", argv[1]);
        return 1;
    }

    if (server_engine (server_port) == -1)
    {
        printf ("SERVER ERROR: Failed to run server engine\n");
        return 1;
    }
    return 0;
}