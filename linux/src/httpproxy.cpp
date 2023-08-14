#include "httpproxy.h"

#include <vector>

bool HttpProxy::sync_proxy()
{
    struct addrinfo* destInfo;

    int status = getaddrinfo(host_.c_str(), http_port_.c_str(), &hints_, &destInfo);
    if (status != 0) {
        std::cout << "getaddrinfo error for destination server: " << gai_strerror(status) << std::endl;
        return false;
    }

    // Create socket for the destination server
    int destSocket = socket(destInfo->ai_family, destInfo->ai_socktype, destInfo->ai_protocol);
    if (destSocket == -1) {
        std::cout << "Socket error for destination server" << std::endl;
        freeaddrinfo(destInfo);
        return false;
    }

    // Connect to the destination server
    if (connect(destSocket, destInfo->ai_addr, destInfo->ai_addrlen) == -1) {
        std::cout << "Connect error for destination server" << std::endl;
        freeaddrinfo(destInfo);
        return false;
    }

    freeaddrinfo(destInfo);

    send(destSocket, request_.c_str(), request_.size(), 0);

    // Forward the response from the destination server back to the client
    char buffer[1024];
    memset(buffer, 0, sizeof(buffer));
    while (true) {
        memset(buffer, 0, sizeof(buffer));
        int bytesReceived = recv(destSocket, buffer, sizeof(buffer), 0);
        if (bytesReceived <= 0) {
            break;
        }
        response_ += std::string(buffer, bytesReceived);
    }
    return true;
}