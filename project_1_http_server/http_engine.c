// NXC Data Communications Network http_engine.c for HTTP server

#include "http_functions.h"
#include "sys/socket.h"
#include "arpa/inet.h"
#include "netinet/tcp.h"

#define MAX_WAITING_CONNECTIONS 10 // Maximum number of waiting connections
#define MAX_PATH_SIZE 256 // Maximum size of path
#define SERVER_ROOT "./server_root"
#define ALBUM_PATH "/public/album"
#define ALBUM_HTML_PATH "./server_root/public/album/album_images.html"
#define ALBUM_HTML_TEMPLATE "<div class=\"card\"> <img src=\"/public/album/%s\" alt=\"Unable to load %s\"> </div>\n"


// You are NOT REQUIRED to implement and use parse_http_header() function for this project.
// However, if you do, you will be able to use the http struct and its member functions,
// which will make things MUCH EASIER for you. We highly recommend you to do so.
// HINT: Use strtok() to tokenize the header strings, based on the delimiters.
http_t *parse_http_header (char *header_str)
{
    http_t *http = init_http();

    return http;
}

// TODO: Initialize server socket and serve incoming connections, using server_routine.
// HINT: Refer to the implementations in socket_util.c from the previous project.
int server_engine (int server_port)
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
// HINT: Your implementation will be MUCH EASIER if you use the functions & structs provided in http_util.c.
//       Please read the descriptions in http_functions.h and the function definitions in http_util.c,
//       and use the provided functions as much as possible!
//       Also, please read https://en.wikipedia.org/wiki/List_of_HTTP_header_fields to get a better understanding
//       on how the structure and protocol of HTTP messages are defined.
int server_routine (int client_sock)
{
    if (client_sock == -1)
        return -1;

    size_t bytes_received = 0;
    char *http_version = "HTTP/1.0"; // We will only support HTTP/1.0 in this project.
    char header_buffer[MAX_HTTP_MSG_HEADER_SIZE] = {0};
    int header_too_large_flag = 0;
    http_t *response = NULL, *request = NULL;

    // TODO: Receive the HEADER of the client http message.
    //       You have to consider the following cases:
    //       1. End of header delimiter is received (HINT: https://en.wikipedia.org/wiki/List_of_HTTP_header_fields)
    //       2. Error occurs on read() (i.e. read() returns -1)
    //       3. Client disconnects (i.e. read() returns 0)
    //       4. MAX_HTTP_MSG_HEADER_SIZE is reached (i.e. message is too long)
    while (1)
    {
        // Remove this line and implement the logic described above.
        header_too_large_flag = 1;
        break;
    }

    // Send different http response depending on the request.
    // Carefully follow the four cases and their TODOs described below.
    // HINT: Please refer to https://developer.mozilla.org/en-US/docs/Web/HTTP/Status for more information on HTTP status codes.
    //       We will be using code 200, 400, 401, 404, and 431 in this project.

    // Case 1: If the received header message is too large... 
    // TODO: Send 431 Request Header Fields Too Large. (IMPLEMENTED)
    // HINT: In most real-world web browsers, this error rarely occurs.
    //       However, we implemented this case to give you a HINT on how to use the included functions in http_util.c.
    if (header_too_large_flag)
    {
        // Create the response, with the appropriate status code and http version.
        // Refer to http_util.c for more details.
        response = init_http_with_arg (NULL, NULL, http_version, "431");
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
    }
    else
    {
        // We have successfully received the header of the client http message.
        // TODO: Parse the header of the client http message.
        // HINT: Implement and use parse_http_header() function to format the received http message into a struct.
        //       You are NOT REQUIRED to implement and use parse_http_header().
        //       However, if you do, you will be able to use the http struct and its member functions,
        //       which will make things MUCH EASIER for you. We highly recommend you to do so.

        request = parse_http_header (header_buffer); // TODO: Change this to your implementation.


        // We must behave differently depending on the type of the request.
        if (strncmp (request->method, "GET", 3) == 0)
        {
            // Case 2: GET request is received.
            // HINT: It is common to return index.html when the client requests a directory.
            
            // TODO: First check if the requested file needs authorization. If so, check if the client is authorized.
            // HINT: The client will send the ID and password in BASE64 encoding in the Authorization header field, 
            //       in the format of "Basic <ID:password>", where <ID:password> is encoded in BASE64.
            //       Refer to https://developer.mozilla.org/ko/docs/Web/HTTP/Authentication for more information.
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
            // TODO: Send 401 Unauthorized with WWW-Authenticate field set to Basic.
            //       Refer to https://developer.mozilla.org/ko/docs/Web/HTTP/Authentication for more information.
        }
        else if (strncmp (request->method, "POST", 4) == 0)
        {
            // Case 3: POST request is received.
            // TODO: Receive the body of the POST http message.
            // HINT: Use the Content-Length & boundary in Content-type field in the header to determine 
            //       the start & the size of the body.
            //       Also, there might be some parts of the body that were received along with the header...
            //       Refer to https://developer.mozilla.org/en-US/docs/Web/HTTP/Methods/POST for more information.

            // TODO: Parse each request_body of the multipart content request_body.

            // TODO: Get the filename of the file.

            // TODO: Check if the file is an image file.

            // TODO: Save the file in the album.

            // Append the appropriate html for the new image to album.html.
            char filename[MAX_PATH_SIZE] = "TODO: Change this to the filename of the image";
            size_t html_append_size = strlen (ALBUM_HTML_TEMPLATE) + strlen (filename)*2 + 1;
            char *html_append = (char *)calloc (1, html_append_size);
            sprintf (html_append, ALBUM_HTML_TEMPLATE, filename, filename);
            append_file (ALBUM_HTML_PATH , html_append, strlen (html_append));
            free (html_append);

            // TODO: Respond with a 200 OK.

        }
        else
        {
            // Case 4: Other requests...
            // TODO: Send 400 Bad Request.
        }
    }

    // Send the response to the client.
    if (response != NULL)
    {
        printf ("\tHTTP ");
        GREEN_PRTF ("RESPONSE:\n");
        print_http_header (response);

        // Parse http response to buffer
        void *response_buffer = NULL;
        ssize_t response_size = write_http_to_buffer (response, &response_buffer);
        if (response_size == -1)
        {
            ERROR_PRTF ("SERVER ERROR: Failed to write HTTP response to buffer\n");
            free_http (request);
            free_http (response);
            return 0;
        }

        // Send http response to client
        if (write_bytes (client_sock, response_buffer, response_size) == -1)
        {
            ERROR_PRTF ("SERVER ERROR: Failed to send response to client\n");
            free (response_buffer);
            free_http (request);
            free_http (response);
            return 0;
        }

        free (response_buffer);
    }
    free_http (request);
    free_http (response);
    return 0;
}
