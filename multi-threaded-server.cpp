#include <iostream>
#include <sstream>
#include <string>
#include <cstring>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fstream>
#include <stdexcept>
#include "ThreadPool.h"

const int PORT = 8080;
const int BUFFER_SIZE = 1024*10;

const int threadPoolSize = 100;
ThreadPool threadPool(threadPoolSize);

// Function to handle client request and send a static HTML file as a response
void handleClient(int clientSocket) {
    try{
        char buffer[BUFFER_SIZE];
        memset(buffer, 0, BUFFER_SIZE);

        // Read the client request
        int bytesRead = read(clientSocket, buffer, BUFFER_SIZE - 1);
        if (bytesRead < 0) {
            std::cerr << "Error reading from socket\n";
            close(clientSocket);
            return;
        }

        // Parse HTTP request (very basic parsing just to extract requested file)
        std::istringstream requestStream(buffer);
        std::string method, path, version;
        requestStream >> method >> path >> version;

        // Remove leading '/' from the path to get the file name
        std::string fileName = path == "/" ? "index.html" : path.substr(1);

        // Open the requested file
        std::ifstream file(fileName);
        std::ostringstream response;

        if (file) {
            // Read file content into response
            std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

            // Send HTTP headers
            response << "HTTP/1.1 200 OK\r\n";
            response << "Content-Type: text/html\r\n";
            response << "Content-Length: " << content.size() << "\r\n";
            response << "\r\n";

            // Send file content
            response << content;
        } else {
            // File not found, return 404 response
            response << "HTTP/1.1 404 Not Found\r\n";
            response << "Content-Type: text/html\r\n";
            response << "\r\n";
            response << "<html><body><h1>404 Not Found! :(</h1></body></html>";
        }

        // Send the response to the client
        std::string responseString = response.str();
        send(clientSocket, responseString.c_str(), responseString.size(), 0);
    }
    catch (const std::exception& e) {
        // print the exception
        std::cout << "An Exception Occured: \n" << e.what() << std::endl;
    }
    
    // Close the client socket
    close(clientSocket);
}

int main() {
    // Create server socket
    int serverSocketFd = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocketFd < 0) {
        std::cerr << "Error creating socket\n";
        return 1;
    }

    // Bind the server socket to a port
    sockaddr_in serverAddr;
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_UNSPEC;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(PORT);

    if (bind(serverSocketFd, (sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        std::cerr << "Error binding socket\n";
        close(serverSocketFd);
        return 1;
    }

    // Start listening for connections
    if (listen(serverSocketFd, 10) < 0) {
        std::cerr << "Error listening on socket\n";
        close(serverSocketFd);
        return 1;
    }

    std::cout << "Server is running on port " << PORT << "\n";

    // Main loop: accept and handle client connections
    while (true) {
        // Accept client connection
        int clientSocket = accept(serverSocketFd, nullptr, nullptr);
        if (clientSocket < 0) {
            std::cerr << "Error accepting connection\n";
            continue;
        }
        
        // Handle client request
        threadPool.enqueueTask([clientSocket] {
            handleClient(clientSocket);
        });
    }

    // Close server socket
    close(serverSocketFd);

    return 0;
}
