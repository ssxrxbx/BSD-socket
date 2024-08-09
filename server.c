/*
A simple server in the internet domain using TCP
Usage:./server port (E.g. ./server 10000 )
*/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>  // definitions of a number of data types used in socket.h and netinet/in.h
#include <sys/socket.h> // definitions of structures needed for sockets, e.g. sockaddr
#include <netinet/in.h> // constants and structures needed for internet domain addresses, e.g. sockaddr_in
#include <strings.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>

// Function to handle errors
void error(char *msg)
{
    perror(msg);
    exit(1);
}

// Function to determine content type based on file extension
char *getContentType(char *path)
{
    const char *extension = strrchr(path, '.');
    if (extension == NULL)
        return NULL;

    extension++; // Skip the dot

    // Check what file extension the extracted extension is.
    if (strcasecmp(extension, "jpeg") == 0)
    {
        return "image/jpeg";
    }
    else if (strcasecmp(extension, "png") == 0)
    {
        return "image/png";
    }
    else if (strcasecmp(extension, "gif") == 0)
    {
        return "image/gif";
    }
    else if (strcasecmp(extension, "pdf") == 0)
    {
        return "application/pdf";
    }
    else if (strcasecmp(extension, "mp3") == 0)
    {
        return "audio/mpeg";
    }
    else if (strcasecmp(extension, "html") == 0 || strcasecmp(extension, "htm") == 0)
    {
        return "text/html";
    }
    return NULL;
}

// Function to handle client requests
void handleRequest(int client_socket)
{
    char buffer[1024];                                           // Buffer for reading client requests
    ssize_t n = read(client_socket, buffer, sizeof(buffer) - 1); // Read request from client
    if (n < 0)
        error("ERROR reading from socket");
    buffer[n] = '\0'; // Null-terminate the string

    printf("HTTP Request is %s\n", buffer); // Print the received HTTP request

    // Extract file path from request message
    char *filename = strtok(buffer, " \r\n"); // Parse the request to get the filename
    if (filename == NULL)
        return;

    filename = strtok(NULL, " \r\n"); // Get the filename part of the request
    if (filename == NULL)
        return;

    // Construct file path
    char file_path[256];                  // Variable to hold the path of the requested file
    sprintf(file_path, "./%s", filename); // Construct the file path using the requested filename

    // Get metadata of the requested file
    struct stat file_stat; // Variable to hold file metadata
    if (stat(file_path, &file_stat) == -1)
    {
        // Send 404 response if file does not exist
        char response[] = "HTTP/1.1 404 Not Found\r\nContent-Type: text/html\r\n\r\n"
                          "<!DOCTYPE HTML>\n"
                          "<html>\n"
                          "<head><title>404 Not Found</title></head>\n"
                          "<body>\n"
                          "<h1>404 Not Found</h1>\n"
                          "<p>The requested URL was not found on this server.</p>\n"
                          "</body>\n"
                          "</html>";
        printf("Response message: %s", response); // Print the response message
        send(client_socket, response, strlen(response), 0); // Send the response to the client
        return;
    }

    if (!S_ISREG(file_stat.st_mode))
    {
        // Send 403 response if requested path is not a regular file
        char response[] = "HTTP/1.1 403 Forbidden\r\nContent-Length: 0\r\n\r\n";
        printf("Response message: %s", response);           // Print the response message
        send(client_socket, response, strlen(response), 0); // Send the response to the client
        return;
    }

    // Open the requested file
    int file = open(file_path, O_RDONLY); // Open the file in read-only mode
    if (file < 0)
    {
        // Send 500 response if file cannot be opened
        char response[] = "HTTP/1.1 500 Internal Server Error\r\nContent-Length: 0\r\n\r\n";
        printf("Response message: %s", response);           // Print the response message
        send(client_socket, response, strlen(response), 0); // Send the response to the client
        return;
    }

    // Send 200 OK response with content type and length
    char response[1024]; // Buffer for constructing the response message
    sprintf(response, "HTTP/1.1 200 OK\r\nContent-Type: %s\r\nContent-Length: %ld\r\n\r\n",
            getContentType(file_path), (long)file_stat.st_size); // Construct the response message
    printf("Response message: %s", response);                    // Print the response message
    send(client_socket, response, strlen(response), 0);          // Send the response to the client

    // Send the file content to the client
    char buffer_out[1024]; // Buffer for reading file content
    ssize_t bytes_read;    // Variable to hold the number of bytes read
    while ((bytes_read = read(file, buffer_out, sizeof(buffer_out))) > 0)
    {
        send(client_socket, buffer_out, bytes_read, 0); // Send the file content to the client
    }

    close(file); // Close the file
}

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        fprintf(stderr, "ERROR, no port provided\n");
        exit(1);
    }

    // Create a socket
    int socketfd = socket(AF_INET, SOCK_STREAM, 0); // Create a new socket
    if (socketfd < 0)
        error("ERROR opening socket");

    struct sockaddr_in serv_addr, cli_addr; // Variables to hold server and client addresses
    int portno = atoi(argv[1]);             // Port number provided as command-line argument

    memset(&serv_addr, 0, sizeof(serv_addr)); // Initialize server address structure
    serv_addr.sin_family = AF_INET;           // Set address family to IPv4
    serv_addr.sin_addr.s_addr = INADDR_ANY;   // Bind socket to any available local address
    serv_addr.sin_port = htons(portno);       // Set port number

    // Bind the socket to the address
    if (bind(socketfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
        error("ERROR on binding");

    // Listen for client connections
    listen(socketfd, 5); // Allow up to 5 pending connections in the queue

    printf("Server: Waiting for client's connection on port %d...\n", portno); // Print server status message

    while (1)
    {
        // Accept client connection
        socklen_t clilen = sizeof(cli_addr);                                         // Length of client address structure
        int client_socket = accept(socketfd, (struct sockaddr *)&cli_addr, &clilen); // Accept client connection
        if (client_socket < 0)
            error("ERROR on accept");

        // Handle client request
        handleRequest(client_socket); // Handle the client request

        // Close the client connection
        close(client_socket); // Close the client socket
    }

    close(socketfd); // Close the server socket
    return 0;
}