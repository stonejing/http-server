#include <iostream>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <string>

const int MAX_EVENTS = 10;
const int MAX_BUFFER_SIZE = 1024;

int main() {
    // Create socket
    int serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == -1) {
        std::cerr << "Failed to create socket" << std::endl;
        return 1;
    }

    // Set up server address
    sockaddr_in serverAddress{};
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = INADDR_ANY;
    serverAddress.sin_port = htons(8000);

    int reuse = 1;
    setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

    // Bind socket to server address
    if (bind(serverSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0) {
        std::cerr << "Failed to bind socket" << std::endl;
        close(serverSocket);
        return 1;
    }

    // Listen for incoming connections
    if (listen(serverSocket, 5) < 0) {
        std::cerr << "Failed to listen for connections" << std::endl;
        close(serverSocket);
        return 1;
    }

    // Create epoll instance
    int epollFd = epoll_create1(0);
    if (epollFd == -1) {
        std::cerr << "Failed to create epoll instance" << std::endl;
        close(serverSocket);
        return 1;
    }

    // Add server socket to epoll
    epoll_event event{};
    event.events = EPOLLIN;
    event.data.fd = serverSocket;
    if (epoll_ctl(epollFd, EPOLL_CTL_ADD, serverSocket, &event) == -1) {
        std::cerr << "Failed to add server socket to epoll" << std::endl;
        close(epollFd);
        close(serverSocket);
        return 1;
    }

    // Main event loop
    epoll_event events[MAX_EVENTS];
    std::string response = "HTTP/1.1 200 OK\r\nConnection: Keep-Alive\r\nContent-Length: 5\r\n\r\nHello";

    while (true) {
        int numEvents = epoll_wait(epollFd, events, MAX_EVENTS, -1);
        if (numEvents == -1) {
            std::cerr << "Failed to wait for events" << std::endl;
            break;
        }

        for (int i = 0; i < numEvents; ++i) {
            if (events[i].data.fd == serverSocket) {
                // Accept incoming connection
                sockaddr_in clientAddress{};
                socklen_t clientAddressLength = sizeof(clientAddress);
                int clientSocket = accept(serverSocket, (struct sockaddr*)&clientAddress, &clientAddressLength);
                if (clientSocket == -1) {
                    std::cerr << "Failed to accept connection" << std::endl;
                    continue;
                }

                // Add client socket to epoll
                event.events = EPOLLIN | EPOLLET;
                event.data.fd = clientSocket;
                if (epoll_ctl(epollFd, EPOLL_CTL_ADD, clientSocket, &event) == -1) {
                    std::cerr << "Failed to add client socket to epoll" << std::endl;
                    close(clientSocket);
                    continue;
                }
            } else {
                // Handle client socket events
                std::cout << "handle listen envent: " << events[i].data.fd << std::endl;
                int clientSocket = events[i].data.fd;
                char buffer[MAX_BUFFER_SIZE];
                ssize_t bytesRead = recv(clientSocket, buffer, sizeof(buffer), 0);
                if (bytesRead == -1) {
                    std::cerr << "Failed to receive data from client" << std::endl;
                    close(clientSocket);
                    continue;
                } else if (bytesRead == 0) {
                    // Client closed the connection
                    std::cout << "Client closed connection" << std::endl;
                    close(clientSocket);
                    continue;
                }

                // Parse request and send response
                std::string request(buffer, bytesRead);
                std::cout << "Received request: " << request << std::endl;

                // Simulate processing time
                // sleep(1);

                ssize_t bytesSent = send(clientSocket, response.c_str(), response.length(), 0);
                if (bytesSent == -1) {
                    std::cerr << "Failed to send response to client" << std::endl;
                }

                // Check for Keep-Alive header
                if (request.find("Connection: keep-alive") == std::string::npos) {
                    // Keep-Alive header not found, close the connection
                    std::cout << "Closing connection" << std::endl;
                    epoll_ctl(epollFd, EPOLL_CTL_DEL, clientSocket, 0);
                    close(clientSocket);
                }
            }
        }
    }

    // Clean up
    close(epollFd);
    close(serverSocket);

    return 0;
}
