#include <iostream>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <string>
#include <queue>
#include <map>

const int MAX_EVENTS = 10;
const int MAX_BUFFER_SIZE = 1024;

struct Client {
    int socket;
    std::queue<std::string> responses;
};

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
    event.events = EPOLLIN | EPOLLET;
    event.data.fd = serverSocket;
    if (epoll_ctl(epollFd, EPOLL_CTL_ADD, serverSocket, &event) == -1) {
        std::cerr << "Failed to add server socket to epoll" << std::endl;
        close(epollFd);
        close(serverSocket);
        return 1;
    }

    // Create a map to store client data
    std::map<int, Client> clients;

    while (true) {
        epoll_event events[MAX_EVENTS];
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

                // Add client data to the map
                clients[clientSocket] = Client{clientSocket, std::queue<std::string>()};
            } else {
                // Handle client socket events
                int clientSocket = events[i].data.fd;

                if (events[i].events & EPOLLIN) {
                    // Read data from client
                    char buffer[MAX_BUFFER_SIZE];
                    ssize_t bytesRead = recv(clientSocket, buffer, sizeof(buffer), 0);
                    if (bytesRead == -1) {
                        std::cerr << "Failed to receive data from client" << std::endl;
                        close(clientSocket);
                        clients.erase(clientSocket);
                        continue;
                    } else if (bytesRead == 0) {
                        // Client closed the connection
                        std::cout << "Client closed connection" << std::endl;
                        close(clientSocket);
                        clients.erase(clientSocket);
                        continue;
                    }

                    // Parse request and process it
                    std::string request(buffer, bytesRead);
                    std::cout << "Received request: " << request << std::endl;

                    // Simulate processing time
                    sleep(1);

                    // Store response in the client's queue
                    clients[clientSocket].responses.push("HTTP/1.1 200 OK\r\nConnection: Keep-Alive\r\nContent-Length: 5\r\n\r\nHello");
                    
                    // Modify the event to include EPOLLOUT
                    event.data.fd = clientSocket;
                    event.events = EPOLLOUT | EPOLLET;
                    if (epoll_ctl(epollFd, EPOLL_CTL_MOD, clientSocket, &event) == -1) {
                        std::cerr << "Failed to modify epoll events for client socket" << std::endl;
                        close(clientSocket);
                        clients.erase(clientSocket);
                        continue;
                    }
                } else if (events[i].events & EPOLLOUT) {
                    // Write data to client
                    std::string& response = clients[clientSocket].responses.front();
                    ssize_t bytesSent = send(clientSocket, response.c_str(), response.length(), 0);
                    if (bytesSent == -1) {
                        std::cerr << "Failed to send response to client" << std::endl;
                        close(clientSocket);
                        clients.erase(clientSocket);
                        continue;
                    }

                    // Remove the sent response from the client's queue
                    clients[clientSocket].responses.pop();

                    // If there are no more responses in the queue, switch back to EPOLLIN
                    if (clients[clientSocket].responses.empty()) {
                        event.data.fd = clientSocket;
                        event.events = EPOLLIN | EPOLLET;
                        if (epoll_ctl(epollFd, EPOLL_CTL_MOD, clientSocket, &event) == -1) {
                            std::cerr << "Failed to modify epoll events for client socket" << std::endl;
                            close(clientSocket);
                            clients.erase(clientSocket);
                            continue;
                        }
                    }
                }
            }
        }
    }

    // Clean up
    for (const auto& [clientSocket, client] : clients) {
        close(clientSocket);
    }
    close(epollFd);
    close(serverSocket);

    return 0;
}
