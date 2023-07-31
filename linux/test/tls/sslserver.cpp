#include <iostream>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <openssl/ssl.h>
#include <openssl/bio.h>
#include <openssl/err.h>

#define CERT_FILE "server.crt"
#define KEY_FILE "server.key"

void handle_client(SSL* ssl) {
    char buffer[1024] = {0};
    int bytes_read = SSL_read(ssl, buffer, sizeof(buffer));
    if (bytes_read > 0) {
        std::cout << "Received request: " << buffer << std::endl;
        const char* response = "HTTP/1.1 200 OK\r\nContent-Length: 13\r\n\r\nHello, world!";
        SSL_write(ssl, response, strlen(response));
    }
    SSL_shutdown(ssl);
    SSL_free(ssl);
}

int main() {
    SSL_load_error_strings();
    SSL_library_init();
    OpenSSL_add_all_algorithms();

    const char* cert_file = CERT_FILE;
    const char* key_file = KEY_FILE;

    SSL_CTX* ctx = SSL_CTX_new(TLS_server_method());
    if (!ctx) {
        std::cerr << "Error creating SSL context." << std::endl;
        return 1;
    }

    if (SSL_CTX_use_certificate_file(ctx, cert_file, SSL_FILETYPE_PEM) <= 0 ||
        SSL_CTX_use_PrivateKey_file(ctx, key_file, SSL_FILETYPE_PEM) <= 0) {
        std::cerr << "Error loading certificate or private key." << std::endl;
        return 1;
    }

    int server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        std::cerr << "Error creating socket." << std::endl;
        return 1;
    }

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(12345);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        std::cerr << "Error binding socket." << std::endl;
        return 1;
    }

    if (listen(server_socket, 5) < 0) {
        std::cerr << "Error listening on socket." << std::endl;
        return 1;
    }

    std::cout << "Server is listening on 127.0.0.1:12345..." << std::endl;

    while (true) {
        struct sockaddr_in client_addr;
        socklen_t client_addr_len = sizeof(client_addr);
        int client_socket = accept(server_socket, (struct sockaddr*)&client_addr, &client_addr_len);
        if (client_socket < 0) {
            std::cerr << "Error accepting connection." << std::endl;
            continue;
        }

        SSL* ssl = SSL_new(ctx);
        if (!ssl) {
            std::cerr << "Error creating SSL object." << std::endl;
            close(client_socket);
            continue;
        }

        SSL_set_fd(ssl, client_socket);
        if (SSL_accept(ssl) <= 0) {
            std::cerr << "Error performing SSL handshake." << std::endl;
            SSL_free(ssl);
            close(client_socket);
            continue;
        }

        std::cout << "Accepted connection from " << inet_ntoa(client_addr.sin_addr)
                  << ":" << ntohs(client_addr.sin_port) << std::endl;

        handle_client(ssl);
    }

    SSL_CTX_free(ctx);
    close(server_socket);
    return 0;
}