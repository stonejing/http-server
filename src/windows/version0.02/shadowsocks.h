#pragma once

#include "winsock2.h"
#include "windows.h"
#include <WS2tcpip.h>

#include <memory>
#include <string>

#include "dbg.h"
#include "crypto.h"

const unsigned char SVERSION            = 0x05;
const unsigned char METHOD_NOAUTH       = 0x00;
const unsigned char METHOD_UNACCEPTABLE = 0xff;

const unsigned char SOCKS5_CMD_CONNECT  = 0x01;
const unsigned char SOCKS5_CMD_BIND     = 0x02;
const unsigned char SOCKS5_CMD_UDP_ASSOCIATE = 0x04;

const unsigned char SOCKS5_ATYP_IPV4    = 0x01;
const unsigned char SOCKS5_ATYP_DOMAIN  = 0x03;
const unsigned char SOCKS5_ATYP_IPV6    = 0x06;

const unsigned char SOCKS5_REP_SUCCEEDED = 0x00;
const unsigned char SOCKS5_REP_GENERAL = 0x01;
const unsigned char SOCKS5_REP_CONN_DISALLOWED = 0x02;
const unsigned char SOCKS5_REP_NETWORK_UNREACHABLE = 0x03;
const unsigned char SOCKS5_REP_HOST_UNREACHABLE = 0x04;
const unsigned char SOCKS5_REP_CONN_REFUSED = 0x05;
const unsigned char SOCKS5_REP_TTL_EXPIRED = 0x06;
const unsigned char SOCKS5_REP_CMD_NOT_SUPPORTED = 0x07;
const unsigned char SOCKS5_REP_ADDRTYPE_NOT_SUPPROTED = 0x08;
const unsigned char SOCKS5_REP_FF_UNASSIGNED = 0x09;

struct method_select_request {
    unsigned char ver;
    unsigned char nmethods;
    unsigned char methods;
};

struct method_select_response {
    unsigned char ver;
    unsigned char method;
};

struct socks5_request {
    unsigned char ver;
    unsigned char cmd;
    unsigned char rsv;
    unsigned char atyp;
};

struct socks5_response {
    unsigned char ver;
    unsigned char rep;
    unsigned char rsv;
    unsigned char atyp;
};

const int STAGE_ERROR       = -1;
const int STAGE_INIT        = 0;
const int STAGE_HANDSHAKE   = 1;
const int STAGE_CONNECT     = 2;
const int STAGE_RESOLVE     = 4;
const int STAGE_STREAM      = 5;
const int STAGE_STOP        = 6;

using std::shared_ptr;
using std::make_shared;
using std::unique_ptr;
using std::make_unique;

class Shadowsocks
{
public:
    Shadowsocks(SOCKET accept_socket, SOCKET connect_socket, 
                std::string address, int port) : 
        accept_socket_(accept_socket),
        connect_socket_(connect_socket),
        stage_(0),
        recv_buffer_len_(0),
        send_buffer_len_(0),
        send_buffer_idx_(0),
        crypto_(make_unique<Crypto>())
    {
        ULONG NonBlock = 1;
        remote_server_.sin_family = AF_INET;
        remote_server_.sin_port = htons(port);
        //remote_server_.sin_addr.S_un.S_addr = inet_pton(address.c_str());
        InetPton(AF_INET, address.c_str(), &remote_server_.sin_addr.S_un.S_addr);
        if(ioctlsocket(accept_socket_, FIONBIO, &NonBlock) == SOCKET_ERROR)
        {
            log_err("can not set socket to nonblock %d", WSAGetLastError());
        }
    }

    ~Shadowsocks()
    {
        closesocket(accept_socket_);
        closesocket(connect_socket_);
    }

    SOCKET get_accept_socket()
    {
        return accept_socket_;
    }

    SOCKET get_connect_socket()
    {
        return connect_socket_;
    }

    int get_write_status()
    {
        return (send_buffer_len_ == 0);
    }

    bool get_direction()
    {
        return direction_;
    }

    int ShadowsocksHandleLocal();
    int ShadowsocksHandleRemote();

    int ShadowsocksSend(SOCKET socket);

private:
    int ShadowsocksStageInit();
    int ShadowsocksStageHandshake();
    int ShadowsocksStageStream();

private:
    SOCKET accept_socket_;
    SOCKET connect_socket_;

    static const int RECV_BUFFER_SIZE = 1024;
    static const int SEND_BUFFER_SIZE = 20 * 1024;

    int stage_;

    struct sockaddr_in remote_server_;

    char recv_buffer_[RECV_BUFFER_SIZE];
    int recv_buffer_len_;

    char send_buffer_[SEND_BUFFER_SIZE];
    int send_buffer_len_;

    int send_buffer_idx_;
    bool direction_; // true for accept, false for connect 

    unique_ptr<Crypto> crypto_;
};