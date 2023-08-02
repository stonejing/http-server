#include <iostream>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h> // for gethostbyname

void proxyClientToServer(int clientSock, const char* targetHost, int targetPort) {
    struct hostent* host = gethostbyname(targetHost);
    if (!host) {
        herror("Error resolving hostname");
        return;
    }

    struct sockaddr_in serverAddr;
    int serverSock = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSock < 0) {
        perror("Error creating server socket");
        return;
    }

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(targetPort);
    serverAddr.sin_addr = *((struct in_addr*)host->h_addr);

    if (connect(serverSock, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        perror("Error connecting to server");
        close(serverSock);
        return;
    }

    char buffer[4096];
    int bytesRead, bytesSent;

    while ((bytesRead = read(clientSock, buffer, sizeof(buffer))) > 0) {
        char* bufferPtr = buffer;
        while (bytesRead > 0) {
            bytesSent = write(serverSock, bufferPtr, bytesRead);
            if (bytesSent <= 0) {
                perror("Error writing to server");
                close(serverSock);
                close(clientSock);
                return;
            }
            bytesRead -= bytesSent;
            bufferPtr += bytesSent;
        }
    }

    close(serverSock);
}

void proxyServerToClient(int clientSock, int serverSock) {
    char buffer[4096];
    int bytesRead, bytesSent;

    while ((bytesRead = read(serverSock, buffer, sizeof(buffer))) > 0) {
        char* bufferPtr = buffer;
        while (bytesRead > 0) {
            bytesSent = write(clientSock, bufferPtr, bytesRead);
            if (bytesSent <= 0) {
                perror("Error writing to client");
                close(serverSock);
                close(clientSock);
                return;
            }
            bytesRead -= bytesSent;
            bufferPtr += bytesSent;
        }
    }
}

int main() {
    const char* targetHost = "info.cern.ch/";
    const int targetPort = 80;
    const int proxyPort = 8000;

    struct sockaddr_in serverAddr, clientAddr;
    socklen_t clientAddrLen = sizeof(clientAddr);
    int serverSock = socket(AF_INET, SOCK_STREAM, 0);

    if (serverSock < 0) {
        perror("Error creating server socket");
        return 1;
    }

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(proxyPort);

    int reuse = 1;
    setsockopt(serverSock, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

    if (bind(serverSock, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        perror("Error binding server socket");
        return 1;
    }

    if (listen(serverSock, 5) < 0) {
        perror("Error listening on server socket");
        return 1;
    }

    std::cout << "Proxy server started on port " << proxyPort << std::endl;

    while (true) {
        int clientSock = accept(serverSock, (struct sockaddr*)&clientAddr, &clientAddrLen);
        if (clientSock < 0) {
            perror("Error accepting client connection");
            continue;
        }

        pid_t pid = fork();

        if (pid < 0) {
            perror("Error forking process");
            close(clientSock);
        } else if (pid == 0) { // Child process (proxy connection to server)
            close(serverSock);
            proxyClientToServer(clientSock, targetHost, targetPort);
            close(clientSock);
            return 0;
        } else { // Parent process (forward server response to client)
            close(clientSock);
            int serverSock = accept(serverSock, (struct sockaddr*)&clientAddr, &clientAddrLen);
            if (serverSock < 0) {
                perror("Error accepting server connection");
                continue;
            }
            proxyServerToClient(clientSock, serverSock);
            close(serverSock);
            close(clientSock);
            return 0;
        }
    }

    close(serverSock);
    return 0;
}
