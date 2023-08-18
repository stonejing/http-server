#include "httpproxy.h"
#include "channel.h"
#include "eventloop.h"
#include "http.h"
#include <ares.h>
#include <netinet/in.h>

void HttpProxy::init(string& request, string& host, ares_channel* channel, EventLoop* loop, Http* http)
{
    ares_channel_ = channel;
    host_ = host;
    loop_ = loop;
    http_ = http;
    request_ = request;
    response_.clear();
    ares_gethostbyname(*ares_channel_, host_.c_str(), AF_INET, &HttpProxy::dns_callback, this);
    // sync_proxy();
}

bool HttpProxy::bufferRead()
{
    vector<char> buffer_(BUFFER_SIZE);
    while(true)
    {
        int bytes_read = ::recv(connect_socket_, buffer_.data(), BUFFER_SIZE, 0);
        if(bytes_read == -1)
        {
            // read buffer empty, wait for another chance
            if(errno == EAGAIN || errno == EWOULDBLOCK)
            {
                return true;
            }
            cout << "read error " << errno << endl;
            return false;
        }
        // remote close connection
        else if(bytes_read == 0)
        {
            cout << "remote close" << endl;
            return false;
        }   
        response_.append(std::string(buffer_.begin(), buffer_.begin() + bytes_read));
    }
    http_->set_response(response_);
    http_->getChannel()->set_write();
    cout << "http proxy response: " << response_ << endl;
    return true;
}

bool HttpProxy::bufferWrite()
{
    cout << "buffer write" << endl;
    send(connect_socket_, request_.c_str(), request_.size(), 0);
    channel_->set_read();
    return false;
}

void HttpProxy::dns_callback(void* arg, int status, int timeouts, struct hostent* host)
{
    if (status == ARES_SUCCESS)
    {
        puts(host->h_name);
        puts(inet_ntoa(*(struct in_addr*)host->h_addr));
        HttpProxy* http_proxy = (HttpProxy*)arg;
        http_proxy->connect_remote((struct in_addr*)host->h_addr);
    }
    else
    {
        std::cout << "lookup failed: " << ares_strerror(status) << std::endl;
    }
}

void HttpProxy::connect_remote(struct in_addr* remote_addr)
{
    cout << "connect remote" << endl;
    struct sockaddr_in dest_addr;
    bzero(&dest_addr, sizeof(dest_addr));
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(80);
    dest_addr.sin_addr = *remote_addr;
    connect_socket_ = socket(AF_INET, SOCK_STREAM, 0);
    int ret = ::connect(connect_socket_, (struct sockaddr*)&dest_addr, sizeof(dest_addr));
    if (ret == -1)
    {
        std::cout << "connect error" << std::endl;
        return;
    }

    send(connect_socket_, request_.c_str(), request_.size(), 0);

    // Forward the response from the destination server back to the client
    char buffer[1024];
    memset(buffer, 0, sizeof(buffer));
    while (true) {
        memset(buffer, 0, sizeof(buffer));
        int bytesReceived = recv(connect_socket_, buffer, sizeof(buffer), 0);
        if (bytesReceived < 0) {
            assert(0);
        }
        else if(bytesReceived == 0)
        {
            cout << "remote close" << endl;
            break;
        }
        response_ += std::string(buffer, bytesReceived);
    }
    http_->set_response(response_);
    http_->getChannel()->set_write();
    channel_ = make_shared<Channel>(loop_, connect_socket_);
    channel_->set_read();
    channel_->set_write_callback(std::bind(&HttpProxy::bufferWrite, this));
    channel_->set_read_callback(std::bind(&HttpProxy::bufferRead, this));
    loop_->setNewChannel(connect_socket_, channel_);
}

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
    close(destSocket);
    return true;
}

