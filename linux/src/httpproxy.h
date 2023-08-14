#pragma once

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

#include "lrucache.h"

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
    }
         
    ~HttpProxy() 
    { 
        // close(connect_socket_); 
    }

    void set_information(string& host, string& URL)
    {
        host_ = host;
        URL_ = URL;
        response_.clear();
    }    

    void set_request(string& request)
    {
        request_ = request;
    }
     
    string get_response()
    {
        sync_proxy();
        return response_;
    }
     
    bool sync_proxy();

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

    LRUCache cache_{102400};
};