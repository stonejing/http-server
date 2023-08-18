#pragma once

#include <memory>
#include <string>
#include <iostream>
#include <map>
#include <fstream>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <filesystem>
#include <unistd.h>
#include <map>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netdb.h>
#include <cstring>
#include <ares.h>
#include <memory.h>

#include "lrucache.h"

class Channel;
class EventLoop;
class Http;

using namespace std;

// get URL and host from HTTP request
// use getaddrinfo get the IP address of URL
// connect to the IP address
// send the HTTP request to the IP address
// receive the HTTP response from the IP address
// send the HTTP response to the client
class HttpProxy
{
public:
    HttpProxy()
    {
        memset(&hints_, 0, sizeof(hints_));
        hints_.ai_family = AF_INET; // IPv4
        hints_.ai_socktype = SOCK_STREAM;
        hints_.ai_flags = AI_PASSIVE;

        header_["connection"] = "close";
        cout << "http proxy constructed" << endl;
    }
         
    ~HttpProxy() 
    { 
        cout << "http proxy destructed" << endl;
        close(connect_socket_); 
    }
    /*
        add ares_channel
        set HOST
        set client request
        call ares_gethostbyname()
    */
    void init(string& request, string& host, ares_channel* channel, EventLoop* loop, Http* http);

    bool bufferRead();
    bool bufferWrite();

    int direction = 5;

    bool bufferReadLocal();
    bool bufferWriteLocal();
    bool bufferReadRemote();
    bool bufferWriteRemote();

    /*
        dns_callback will execute after ares_process or ares_process_fd called 
        connect to the IP address，get connect fd, connect 非阻塞
        注册 channel，设置回调函数
    */
    static void dns_callback(void* arg, int status, int timeouts, struct hostent* host);  
    void connect_remote(struct in_addr* remote_addr); 
    string get_response()
    {
        // sync_proxy();
        return response_;
    }
     
    bool sync_proxy();
    EventLoop* loop_;

private:
    string URL_;
    string host_;
    bool connected_ = false;
    map<string, string> header_;
    string response_;
    string request_;
    int connect_socket_;
    const string http_port_ = "80";

    struct addrinfo hints_;
    ares_channel* ares_channel_;

    std::shared_ptr<Channel> channel_;

    Http* http_;

    LRUCache cache_{102400};
};