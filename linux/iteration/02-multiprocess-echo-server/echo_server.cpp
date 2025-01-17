#include <iostream>
#include <string>
#include <vector>
#include <array>
#include <system_error>
#include <memory>
#include <span>

#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/wait.h>

namespace network {

class Socket {
public:
    Socket(const std::string& ip, uint16_t port) {
        fd_ = socket(AF_INET, SOCK_STREAM, 0);
        if (fd_ < 0) {
            throw std::system_error(errno, std::system_category(), "socket creation failed");
        }

        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        inet_pton(AF_INET, ip.c_str(), &addr.sin_addr);
        addr.sin_port = htons(port);

        int reuse = 1;
        setsockopt(fd_, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

        if (bind(fd_, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
            throw std::system_error(errno, std::system_category(), "bind failed");
        }
    }
    
    explicit Socket(int fd) : fd_(fd) {}

    ~Socket() {
        if (fd_ >= 0) close(fd_);
    }

    void listen(int backlog = 5) {
        if (::listen(fd_, backlog) < 0) {
            throw std::system_error(errno, std::system_category(), "listen failed");
        }
    }

    std::span<const uint8_t> read() {
        ssize_t n = recv(fd_, buffer_.data(), buffer_.size(), 0);
        if (n < 0) {
            throw std::system_error(errno, std::system_category(), "read failed");
        }
        return std::span<const uint8_t>(buffer_.data(), n);
    }

    void write(std::span<const uint8_t> data) {
        size_t total_written = 0;
        while (total_written < data.size()) {
            ssize_t n = send(fd_, data.data() + total_written, 
                           data.size() - total_written, 0);
            if (n < 0) {
                throw std::system_error(errno, std::system_category(), "write failed");
            }
            total_written += n;
        }
    }

    void write(const std::vector<uint8_t>& data) {
        size_t total_written = 0;
        while (total_written < data.size()) {
            ssize_t n = send(fd_, data.data() + total_written, 
                           data.size() - total_written, 0);
            if (n < 0) {
                throw std::system_error(errno, std::system_category(), "write failed");
            }
            total_written += n;
        }
    }

    std::unique_ptr<Socket> accept() {
        sockaddr_in client_addr{};
        socklen_t len = sizeof(client_addr);
        int client_fd = ::accept(fd_, reinterpret_cast<sockaddr*>(&client_addr), &len);
        if (client_fd < 0) {
            throw std::system_error(errno, std::system_category(), "accept failed");
        }
        return std::make_unique<Socket>(client_fd);
    }

private:
    static constexpr size_t BUFFER_SIZE = 1024;
    std::array<uint8_t, BUFFER_SIZE> buffer_;
    int fd_ = -1;
};

class EchoServer {
public:
    EchoServer(const std::string& ip, uint16_t port) 
        : socket_(ip, port) {
        socket_.listen();
    }

    void run() {
        while (true) {
            auto client = socket_.accept();
            if (fork() == 0) {
                handle_client(std::move(client));
                exit(0);
            }
            // wait(nullptr);
            // handle_client(std::move(client));
        }
    }

private:
    void handle_client(std::unique_ptr<Socket> client) {
        try {
            auto data = client->read();
            std::cout << "Received: " << std::string(data.begin(), data.end()) << std::endl;
            client->write(data);
        } catch (const std::exception& e) {
            std::cerr << "Error handling client: " << e.what() << std::endl;
        }
    }

    Socket socket_;
};

} // namespace network

void sigchld_handler(int signo) {
    while (waitpid(-1, NULL, WNOHANG) > 0);
}

int main() {
    signal(SIGCHLD, sigchld_handler);
    try {
        network::EchoServer server("127.0.0.1", 8000);
        server.run();
    } catch (const std::exception& e) {
        std::cerr << "Server error: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}