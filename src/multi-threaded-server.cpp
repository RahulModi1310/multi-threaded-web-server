#include <iostream>
#include <sstream>
#include <string>
#include <cstring>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <unistd.h>
#include <fstream>
#include <stdexcept>
#include <sys/epoll.h>
#include<mutex>
#include<condition_variable>
#include "ThreadPool.h"
#include "LRUCache.h"

const std::string ASSETS_PATH = "assets/";
const int MAX_EPOLL_EVENTS = 3000;
const int PORT = 8080;
const int BUFFER_SIZE = 1024 * 10;
const int THREADPOOL_SIZE = 70;
const int CACHE_SIZE = 6;

ThreadPool threadPool(THREADPOOL_SIZE);


std::mutex mut;
std::condition_variable cv;
LRUCache<std::string, std::string> lru(CACHE_SIZE);
std::string getFileContentFromCache(std::string filePath){
    std::unique_lock<std::mutex> lock(mut);
    auto [fileContent, status] = lru.get(filePath);
    lock.unlock();
    cv.notify_all();
    std::cout<<filePath<<std::endl;
    if(status == MISSED) {
        std::cout<<"MISSED 1"<<std::endl;
        std::unique_lock<std::mutex> lock(mut);
        std::ifstream file(filePath);
        if(!file) {
            return "";
        }
        std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        lru.add(filePath, content);
        fileContent = content;
        file.close();
        lock.unlock();
        cv.notify_all();
    }
    return fileContent;
}

std::string getContentType(const std::string& fileName) {
    if (fileName.ends_with(".html")) return "text/html";
    if (fileName.ends_with(".css")) return "text/css";
    if (fileName.ends_with(".js")) return "application/javascript";
    if (fileName.ends_with(".png")) return "image/png";
    if (fileName.ends_with(".jpg") || fileName.ends_with(".jpeg")) return "image/jpeg";
    if (fileName.ends_with(".gif")) return "image/gif";
    if (fileName.ends_with(".svg")) return "image/svg+xml";
    if (fileName.ends_with(".ico")) return "image/x-icon";
    if (fileName.ends_with(".json")) return "application/json";
    return "text/plain"; // Default content type
}


// Function to handle client request and send a static HTML file as a response
void handleClient(int clientSocketFd)
{
    try
    {
        char buffer[BUFFER_SIZE];
        memset(buffer, 0, BUFFER_SIZE);

        // Read the client request
        int bytesRead = read(clientSocketFd, buffer, BUFFER_SIZE - 1);
        if (bytesRead < 0)
        {
            std::cerr << "Error reading from socket\n";
            close(clientSocketFd);
            return;
        }

        // Parse HTTP request (very basic parsing just to extract requested file)
        std::istringstream requestStream(buffer);
        std::string method, path, version;
        requestStream >> method >> path >> version;

        // Remove leading '/' from the path to get the file name
        std::string fileName = path == "/" ? "index.html" : path.substr(1);
        std::string filePath = ASSETS_PATH + fileName;

        // get requested file content
        std::string fileContent = getFileContentFromCache(filePath);
        std::ostringstream response;

        if (fileContent.length()>0)
        {
            response << "HTTP/1.1 200 OK\r\n";
            response << "Content-Type: " << getContentType(fileName) << "\r\n";
            response << "Content-Length: " << fileContent.size() << "\r\n";
            response << "\r\n";

            // Send file content
            response << fileContent;
        }
        else
        {
            // File not found, return 404 response
            response << "HTTP/1.1 404 Not Found\r\n";
            response << "Content-Type: text/html\r\n";
            response << "\r\n";
            response << "<html><body><h1>404 Not Found! :(</h1></body></html>";
        }

        // Send the response to the client
        std::string responseString = response.str();
        send(clientSocketFd, responseString.c_str(), responseString.size(), 0);
    }
    catch (const std::exception &e)
    {
        // print the exception
        std::cout << "An Exception Occured: \n"
                  << e.what() << std::endl;
    }

    // Close client socket
    close(clientSocketFd);
}

void setNonBlocking(int sock) {
    fcntl(sock, F_SETFL, fcntl(sock, F_GETFL) | O_NONBLOCK);
}

int main()
{
    // Create server socket
    int serverSocketFd = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocketFd < 0)
    {
        std::cerr << "Error creating socket\n";
        return 1;
    }
    setNonBlocking(serverSocketFd);

    // Bind the server socket to a port
    sockaddr_in serverAddr;
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_UNSPEC;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(PORT);

    if (bind(serverSocketFd, (sockaddr *)&serverAddr, sizeof(serverAddr)) < 0)
    {
        std::cerr << "Error binding socket\n";
        close(serverSocketFd);
        return 1;
    }

    // Start listening for connections
    if (listen(serverSocketFd, 10) < 0)
    {
        std::cerr << "Error listening on socket\n";
        close(serverSocketFd);
        return 1;
    }

    int epollfd = epoll_create1(0);
    if (epollfd == -1)
    {
        perror("epoll_create1");
        exit(EXIT_FAILURE);
    }

    struct epoll_event ev, ready_events[MAX_EPOLL_EVENTS];
    ev.events = EPOLLIN;
    ev.data.fd = serverSocketFd;
    if (epoll_ctl(epollfd, EPOLL_CTL_ADD, serverSocketFd, &ev) == -1)
    {
        perror("epoll_ctl: listen_sock");
        exit(EXIT_FAILURE);
    }

    std::cout << "Server is running on port " << PORT << "\n";

    // Main loop: accept and handle client connections
    while (true)
    {
        int nfds = epoll_wait(epollfd, ready_events, MAX_EPOLL_EVENTS, -1);
        // std::cout << "nfds " << nfds << "\n";
        if (nfds == -1)
        {
            perror("epoll_wait");
            exit(EXIT_FAILURE);
        }

        for (int n = 0; n < nfds; ++n)
        {
            if (ready_events[n].data.fd == serverSocketFd)
            {
                // Accept client connection
                int clientSocketFd = accept(serverSocketFd, nullptr, nullptr);
                if (clientSocketFd < 0)
                {
                    std::cerr << "Error accepting connection\n";
                    continue;
                }

                ev.events = EPOLLIN | EPOLLET;
                ev.data.fd = clientSocketFd;
                if (epoll_ctl(epollfd, EPOLL_CTL_ADD, clientSocketFd, &ev) == -1)
                {
                    perror("epoll_ctl: listen_sock");
                    exit(EXIT_FAILURE);
                }
            }
            else
            {
                // std::cout << "REQUEST HANDLING...\n";
                // Handle client request
                threadPool.enqueueTask([clientSocketFd = ready_events[n].data.fd]
                                       { handleClient(clientSocketFd); });
            }
        }
    }

    // Close server socket
    close(serverSocketFd);

    return 0;
}