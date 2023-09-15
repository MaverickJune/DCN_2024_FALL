// NXC Data Communications http_server.c for HTTP server
// Written by Jongseok Park (cakeng@snu.ac.kr)
// 2023. 9. 11

#include "http_functions.h"
#include "sys/socket.h"
#include "arpa/inet.h"
#include "netinet/tcp.h"

#define MAX_WAITING_CONNECTIONS 10 // Maximum number of waiting connections
#define MAX_PATH_SIZE 256 // Maximum size of path
#define SERVER_ROOT "./server_root"

// Here you can specify which implementation to use.
// Hint: Use the _ans implementations to test your code.
//       For example, if you specify server_engine to use server_engine_ans,
//       and server_routine to use server_routine_problem, 
//       then the server will use the TA implementation of server_engine,
//       and your implementation of server_routine.
//       This will allow you to test only the server_routine function of your implementation.

int server_engine (int server_port)
{
    // You can change this to server_engine_ans to check the behavior of the example implementation.
    // However, MAKE SURE to change it to server_engine_problem when submitting your implementation.
    return server_engine_ans (server_port);
}
int server_routine (int client_sock)
{
    // You can change this to server_routine_ans to check the behavior of the example implementation.
    // However, MAKE SURE to change it to server_routine_problem when submitting your implementation.

    // Also, server_routine_ans will print out the HTTP requests and responses.
    // However, you DO NOT have to print them for your implementation.
    return server_routine_ans (client_sock);
}


// TODO: Initialize server socket and serve incoming connections, using server_routine.
// HINT: Refer to the implementations in socket_util.c from the previous project.
int server_engine_problem (int server_port)
{
    int server_listening_sock = -1;
    // TODO: Initialize server socket

    // TODO: Set socket options to reuse the port immediately after the connection is closed

    // TODO: Bind server socket to the given port
    

    // TODO: Listen for incoming connections

    // Serve incoming connections forever
    while (1)
    {
        struct sockaddr_in client_addr_info;
        socklen_t client_addr_info_len = sizeof(client_addr_info);
        int client_connected_sock = -1;

        // TODO: Accept incoming connections

        // Serve the client
        server_routine (client_connected_sock);
        
        // TODO: Close the connection with the client

    }

    // TODO: Close the server socket

    return 0;
}

// TODO: Implement server routine for HTTP/1.0.
//       Return -1 if error occurs, 0 otherwise.
// HINT: Your implementation will be MUCH EASIER if you use the functions provided in http_util.c.
//       Please read the function descriptions in http_functions.h and the function definition in http_util.c,
//       and use the provided functions as much as possible!
//       Also, please read https://en.wikipedia.org/wiki/List_of_HTTP_header_fields to get a better understanding
//       on how the structure and protocol of HTTP messages are defined.
int server_routine_problem (int client_sock)
{
    size_t bytes_received = 0;
    char *http_version = "HTTP/1.0";
    char header_buffer[MAX_HTTP_MSG_HEADER_SIZE] = {0};
    int header_too_large_flag = 0;

    // TODO: Receive the HEADER of the client http message.
    //       You have to consider the following cases:
    //       1. End of header section is received (HINT: https://en.wikipedia.org/wiki/List_of_HTTP_header_fields)
    //       2. Error occurs on read() (i.e. read() returns -1)
    //       3. Client disconnects (i.e. read() returns 0)
    //       4. MAX_HTTP_MSG_HEADER_SIZE is reached (i.e. message is too long)
    while (1)
    {
        // Remove these lines and implement the logic described above.
        header_too_large_flag = 1;
        break;
    }

    // Send different http response depending on the request.
    // Carefully follow the Cases and TODOs described below.
    // HINT: Please refer to https://developer.mozilla.org/en-US/docs/Web/HTTP/Status for more information on HTTP status codes.
    //       We will be using code 200, 400, 401, 404, and 431 in this project.

    // Case 1: If the received header message is too large... 
    // TODO: Send 431 Request Header Fields Too Large. (IMPLEMENTED)
    // HINT: In most real-world web browsers, this error rarely occurs.
    //       However, we implemented this case to give you a hint on how to code your server.
    if (header_too_large_flag)
    {
        // Create the response, with the appropriate status code and http version.
        // Refer to http_util.c for more details.
        http_t *response = init_http_with_arg (NULL, NULL, http_version, "431");
        if (response == NULL)
        {
            printf ("SERVER ERROR: Failed to create HTTP response\n");
            return -1;
        }
        // Add the appropriate fields to the header of the response.
        add_field_to_http (response, "Content-Type", "text/html");
        add_field_to_http (response, "Connection", "close");

        // Generate and add the body of the response.
        char body[] = "<html><body><h1>431 Request Header Fields Too Large</h1></body></html>";
        add_body_to_http (response, sizeof(body), body);

        printf ("SERVER: Sending HTTP response:\n");
        print_http_header (response);

        // Write the http response to a buffer.
        void *response_buffer = NULL;
        ssize_t response_size = write_http_to_buffer (response, &response_buffer);
        if (response_size == -1)
        {
            printf ("SERVER ERROR: Failed to write HTTP response to buffer\n");
            return -1;
        }

        // Send the http response to the client.
        if (write (client_sock, response_buffer, response_size) == -1)
        {
            printf ("SERVER ERROR: Failed to send HTTP response\n");
            return -1;
        }

        // Free the buffer and the http response.
        free (response_buffer);
        free_http (response);
    }

    // We have successfully received the header of the client http message.
    // TODO: Parse the header of the client http message.
    // HINT: Use parse_http_header() function in http_util.c.
    //       You can also use print_http_header() function to see the received http message after parsing.


    // We must behave differently depending on the type of the request.
    // Case 2: GET request is received.
    // TODO: First check if the requested file needs authorization.
    // HINT: The client sends the ID and password in BASE64 encoding in the Authorization field, 
    //       in the format of "Basic <ID:password>", where <ID:password> is encoded in BASE64.

        int auth_flag = 0;
        char *auth_list[] = {"/secret.html", "/public/images/khl.jpg"};
        char ans_plain[] = "DCN:FALL2023"; // ID:password (Please do not change this.)

        // Case 2-1: If authorization succeeded...
        // TODO: Get the file path from the request.

            // Case 2-1-1: If the file does not exist...
            // TODO: Send 404 Not Found.

            // Case 2-1-2: If the file exists...
            // TODO: Send 200 OK with the file as the body.


        // Case 2-2: If authorization failed...
        // TODO: Send 401 Unauthorized with WWW-Authenticate field.


    // Case 3: Other requests...
    // TODO: Send 400 Bad Request.

    return 0;
}
