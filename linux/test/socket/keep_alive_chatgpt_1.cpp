#include <arpa/inet.h>
#include <cstring>
#include <iostream>
#include <map>
#include <netinet/in.h>
#include <queue>
#include <string>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>

#include "bind_and_listen.h"

const int MAX_EVENTS = 10;
const int MAX_BUFFER_SIZE = 1024;

struct Client 
{
    int socket;
    std::queue<std::string> responses;
};

int main() 
{
    // Create socket
    int serverSocket = bind_and_listen();

    // Create epoll instance
    int epollFd = epoll_create1(0);
    if (epollFd == -1) 
    {
        std::cerr << "Failed to create epoll instance" << std::endl;
        close(serverSocket);
        return 1;
    }

    // Add server socket to epoll
    epoll_event event{};
    event.events = EPOLLIN | EPOLLET;
    event.data.fd = serverSocket;
    if (epoll_ctl(epollFd, EPOLL_CTL_ADD, serverSocket, &event) == -1) 
    {
        std::cerr << "Failed to add server socket to epoll" << std::endl;
        close(epollFd);
        close(serverSocket);
        return 1;
    }

    // Create a map to store client data
    std::map<int, Client> clients;
    epoll_event events[MAX_EVENTS];
    while (true) 
    {
        int numEvents = epoll_wait(epollFd, events, MAX_EVENTS, -1);
        if (numEvents == -1) 
        {
            std::cerr << "Failed to wait for events" << std::endl;
            break;
        }

        for (int i = 0; i < numEvents; ++i) 
        {
            std::cout << "client socket: " << events[i].data.fd << std::endl;
            if (events[i].data.fd == serverSocket) 
            {
                std::cout << "handle accpet" << std::endl;
                // Accept incoming connection
                sockaddr_in clientAddress;
                socklen_t clientAddressLength = sizeof(clientAddress);
                int clientSocket = accept(serverSocket, (struct sockaddr *)&clientAddress,
                        &clientAddressLength);
                if (clientSocket == -1) 
                {
                    std::cerr << "Failed to accept connection" << std::endl;
                    continue;
                }
                std::cerr << "accpet new socket: " << clientSocket << std::endl;
                // Add client socket to epoll
                addfd(epollFd, clientSocket, true);
                // Add client data to the map
                clients[clientSocket] = Client{clientSocket, std::queue<std::string>()};
            } 
            else 
            {
                int clientSocket = events[i].data.fd;
                std::cout << "handle client socket: " << clientSocket << std::endl;
                // Handle client socket events
                if (events[i].events & EPOLLIN) 
                {
                    std::cout << "handle epoll in" << std::endl;
                    // Read data from client
                    char buffer[MAX_BUFFER_SIZE];
                    ssize_t bytesRead = recv(clientSocket, buffer, sizeof(buffer), 0);
                    if (bytesRead == -1) 
                    {
                        std::cerr << "Failed to receive data from client" << std::endl;
                        close(clientSocket);
                        clients.erase(clientSocket);
                        continue;
                    } 
                    else if (bytesRead == 0) 
                    {
                        // Client closed the connection
                        std::cout << "Client closed connection" << std::endl;
                        close(clientSocket);
                        clients.erase(clientSocket);
                        continue;
                    }

                    // Parse request and process it
                    // std::string request(buffer, bytesRead);
                    // std::cout << "Received request: " << request << std::endl;
                    // Simulate processing time
                    // sleep(1);

                    // Store response in the client's queue
                    // clients[clientSocket].responses.push("HTTP/1.1 200 OK\r\nConne \
                    //             ction: Keep-Alive\r\nContent-Length: 5\r\n\r\nHello");

                    // Modify the event to include EPOLLOUT
                    // epoll_event event;
                    // event.data.fd = clientSocket;
                    // event.events = EPOLLOUT | EPOLLET;
                    // epoll_ctl(epollFd, EPOLL_CTL_MOD, clientSocket, &event);
                    modfd(epollFd, clientSocket, EPOLLOUT|EPOLLET);
                } 
                else if (events[i].events & EPOLLOUT) 
                {
                    // Write data to client
                    // std::string &response = clients[clientSocket].responses.front();
                    std::cout << "handle epoll out" << std::endl;
                    std::string response = "HTTP/1.1 200 OK\r\nConne \
                                ction: Keep-Alive\r\nContent-Length: 5\r\n\r\nHello";
                    ssize_t bytesSent = send(clientSocket, response.c_str(), response.length(), 0);
                    if (bytesSent == -1) 
                    {
                        std::cerr << "Failed to send response to client" << std::endl;
                        close(clientSocket);
                        clients.erase(clientSocket);
                        continue;
                    }
                
                    // Remove the sent response from the client's queue
                    // clients[clientSocket].responses.pop();

                    // If there are no more responses in the queue, switch back to EPOLLIN
                    // if (clients[clientSocket].responses.empty()) 
                    // {
                        modfd(epollFd, clientSocket, EPOLLIN);
                    // }
                }
            }
        }
    }

    // Clean up
    for (const auto &[clientSocket, client] : clients) 
    {
        close(clientSocket);
    }
    close(epollFd);
    close(serverSocket);

    return 0;
}
