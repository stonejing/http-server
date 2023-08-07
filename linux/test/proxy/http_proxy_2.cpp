#include <iostream>
#include <string>
#include <cstring>
#include <cstdlib>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netdb.h>

const int BUFFER_SIZE = 4096;

using namespace std;

// Function to parse the destination host and port from an HTTP request
bool parseHttpRequest(const std::string& request, std::string& host, std::string& port) {
    // Parse the host and port from the request (assuming simple format)
    size_t hostStart = request.find("Host: ");
    // if (hostStart != std::string::npos) {
    //     hostStart += 6;
    //     size_t hostEnd = request.find("\r\n", hostStart);
    //     if (hostEnd != std::string::npos) {
    //         std::string hostLine = request.substr(hostStart, hostEnd - hostStart);
    //         size_t portStart = hostLine.find(":");
    //         if (portStart != std::string::npos) {
    //             host = hostLine.substr(0, portStart);
    //             port = hostLine.substr(portStart + 1);
    //             return true;
    //         }
    //     }
    // }
    host = "www.stonejing.link";
    port = "80";
    return true;
}

int main() {
    const char* LISTEN_PORT = "8888"; // Port on which the proxy will listen

    struct addrinfo hints, *res;

    // Set up hints for getaddrinfo
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET; // IPv4
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    // Get address info for the proxy
    int status = getaddrinfo(nullptr, LISTEN_PORT, &hints, &res);
    if (status != 0) {
        std::cerr << "getaddrinfo error: " << gai_strerror(status) << std::endl;
        return 1;
    }

    // Create socket for the proxy server
    int serverSocket = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (serverSocket == -1) {
        std::cerr << "Socket error" << std::endl;
        return 1;
    }
    int reuse = 1;
    setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
    // Bind the socket to the specified port
    if (bind(serverSocket, res->ai_addr, res->ai_addrlen) == -1) {
        std::cerr << "Bind error" << std::endl;
        close(serverSocket);
        return 1;
    }

    freeaddrinfo(res);

    // Start listening for incoming connections
    if (listen(serverSocket, 10) == -1) {
        std::cerr << "Listen error" << std::endl;
        close(serverSocket);
        return 1;
    }

    std::cout << "Proxy is listening on port " << LISTEN_PORT << std::endl;

    while (true) {
        // Accept incoming connection
        struct sockaddr_in clientAddr;
        socklen_t clientAddrSize = sizeof(clientAddr);
        int clientSocket = accept(serverSocket, (struct sockaddr*)&clientAddr, &clientAddrSize);
        if (clientSocket == -1) {
            std::cerr << "Accept error" << std::endl;
            continue;
        }

        // Read the client's HTTP request
        char buffer[BUFFER_SIZE];
        memset(buffer, 0, sizeof(buffer));
        int bytesRead = recv(clientSocket, buffer, sizeof(buffer), 0);
        if (bytesRead == -1) {
            std::cerr << "Receive error" << std::endl;
            close(clientSocket);
            continue;
        }

        std::string request(buffer, bytesRead);

        // Parse the destination host and port from the request
        std::string host, port;
        if (!parseHttpRequest(request, host, port)) {
            std::cerr << "Error parsing request" << std::endl;
            close(clientSocket);
            continue;
        }

        // Get address info for the destination server
        struct addrinfo* destInfo;
        status = getaddrinfo(host.c_str(), port.c_str(), &hints, &destInfo);
        if (status != 0) {
            std::cerr << "getaddrinfo error for destination server: " << gai_strerror(status) << std::endl;
            close(clientSocket);
            continue;
        }

        // Create socket for the destination server
        int destSocket = socket(destInfo->ai_family, destInfo->ai_socktype, destInfo->ai_protocol);
        if (destSocket == -1) {
            std::cerr << "Socket error for destination server" << std::endl;
            freeaddrinfo(destInfo);
            close(clientSocket);
            continue;
        }

        // Connect to the destination server
        if (connect(destSocket, destInfo->ai_addr, destInfo->ai_addrlen) == -1) {
            std::cerr << "Connect error for destination server" << std::endl;
            freeaddrinfo(destInfo);
            close(clientSocket);
            close(destSocket);
            continue;
        }

        freeaddrinfo(destInfo);
        cout << request << endl;
        // Forward the client's HTTP request to the destination server
        if (send(destSocket, request.c_str(), request.size(), 0) == -1) {
            std::cerr << "Send error to destination server" << std::endl;
            close(clientSocket);
            close(destSocket);
            continue;
        }

        // Forward the response from the destination server back to the client
        while (true) {
            memset(buffer, 0, sizeof(buffer));
            int bytesReceived = recv(destSocket, buffer, sizeof(buffer), 0);
            if (bytesReceived <= 0) {
                break;
            }
            
            cout << buffer << endl;

            if (send(clientSocket, buffer, bytesReceived, 0) == -1) {
                std::cerr << "Send error to client" << std::endl;
                break;
            }
        }

        close(clientSocket);
        close(destSocket);
    }

    close(serverSocket);
    return 0;
}
