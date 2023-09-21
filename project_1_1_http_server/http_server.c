// NXC Data Communications http_server.c for HTTP server
// Written by Jongseok Park (cakeng@snu.ac.kr)
// 2023. 9. 11


///// DO NOT MODIFY THIS FILE!! ////

#include "http_functions.h"

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
        ERROR_PRT ("ERROR: Invalid port number: %s\n", argv[1]);
        return 1;
    }

    printf ("Initializing HTTP server...\n");
    if (server_engine_ans (server_port) == -1)
    {
        ERROR_PRT ("SERVER ERROR: Failed to run server engine\n");
        return 1;
    }
    return 0;
}