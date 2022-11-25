#include "shadowsocks.h"
#include <iostream>

using std::cerr;

int Shadowsocks::ShadowsocksSend(SOCKET socket)
{
    int r = send(socket, send_buffer_ + send_buffer_idx_, 
                    send_buffer_len_, 0);
    if(r == -1)
    {
        if(WSAGetLastError() == WSAEWOULDBLOCK)
        {
            return 1;
        }
        else
        {
            return -1;
        }
    }
    else if(r < send_buffer_len_)
    {
        send_buffer_idx_ += r;
        send_buffer_len_ -= r;
        return 1;
    }
    send_buffer_idx_ = 0;
    send_buffer_len_ = 0;
    return 1;
}

int Shadowsocks::ShadowsocksHandleLocal()
{
    int r = recv(accept_socket_, recv_buffer_ + recv_buffer_len_, 
                    1024 - recv_buffer_len_, 0);
    if(r == 0)
    {
        return -1;
    }
    else if(r == SOCKET_ERROR)
    {
        if(WSAGetLastError() == WSAEWOULDBLOCK)
        {
            LOG_INFO << "handle local WSAEWOULDBLOCK." << "\n";
            return 1;
        }
        else
        {
            return -1;
        }
    }
    recv_buffer_len_ += r;

    if(stage_ == STAGE_STREAM)
    {
        if(ShadowsocksStageStream() == -1)
            return -1;
    }
    else if(stage_ == STAGE_INIT)
    {
        if(ShadowsocksStageInit() == -1)
            return -1;
        stage_ = STAGE_HANDSHAKE;
    }
    else if(stage_ == STAGE_HANDSHAKE)
    {
        if(ShadowsocksStageHandshake() == -1)
            return -1;
        stage_ = STAGE_STREAM;
    }
    else
    {
        LOG_ERROR << "unknown happened stage: " << stage_ << "\n";
        return -1;
    }
    recv_buffer_len_ = 0;
    return 1;
}

int Shadowsocks::ShadowsocksStageInit()
{
    if(recv_buffer_len_ < (int)sizeof(struct method_select_request))
        return -1;
    if(recv_buffer_[0] != SVERSION)
        return -1;
    
    struct method_select_response response;
    response.ver = SVERSION;
    response.method = METHOD_NOAUTH;
    char* send_buf = (char*)&response;
    int r = send(accept_socket_, send_buf, sizeof(response), 0);
    if(r != sizeof(response))
        return -1;
    return 1;
}

int Shadowsocks::ShadowsocksStageHandshake()
{
    struct socks5_request* request = (struct socks5_request*)recv_buffer_;
    int request_len = (int)sizeof(struct socks5_request);

    if(recv_buffer_len_ < request_len + 2)
        return -1;
    
    struct socks5_response response;
    response.ver = SVERSION;
    response.rep = SOCKS5_REP_SUCCEEDED;
    response.rsv = 0;
    response.atyp = SOCKS5_ATYP_IPV4;

    if(request->cmd == SOCKS5_CMD_UDP_ASSOCIATE)
        return -1;
    else if(request->cmd != SOCKS5_CMD_CONNECT)
    {
        LOG_ERROR << "unsupported command: " << request->cmd << "\n";
        response.rep = SOCKS5_REP_CMD_NOT_SUPPORTED;
        char* send_buf = (char*)&response;
        send(accept_socket_, send_buf, 4, 0);
        return -1;
    }

    if(::connect(connect_socket_, (SOCKADDR*)&remote_server_, sizeof(remote_server_)) < 0)
    {
        LOG_ERROR << "connect remote error " << WSAGetLastError() << "\n";
        return -1;
    }

    ULONG NonBlock = 1;
    if(ioctlsocket(connect_socket_, FIONBIO, &NonBlock) == SOCKET_ERROR)
    {
        LOG_ERROR << "can not set socket to nonblock " << WSAGetLastError() << "\n";
    }

    if(crypto_->aead_encrypt(recv_buffer_ + 3, recv_buffer_len_ - 3, 
                            send_buffer_, send_buffer_len_) == -1)
    {
        LOG_ERROR << "connect encryption error." << "\n";
        return -1;
    }

    send(connect_socket_, send_buffer_, send_buffer_len_, 0);
    send_buffer_len_ = 0;
    recv_buffer_[1] = SOCKS5_REP_SUCCEEDED;

    send(accept_socket_, recv_buffer_, recv_buffer_len_, 0);

    return 1;
}

int Shadowsocks::ShadowsocksStageStream()
{
    if(crypto_->aead_encrypt(recv_buffer_, recv_buffer_len_, 
            send_buffer_, send_buffer_len_) == -1)
    {
        return -1;
    }
    int s = send(connect_socket_, send_buffer_, send_buffer_len_, 0);
    if(s == -1)
    {
        if(WSAGetLastError() == WSAEWOULDBLOCK)
        {
            direction_ = true;
            return 1;
        }
        else
        {
            return -1;
        }
    }
    else if(s < send_buffer_len_)
    {
        send_buffer_idx_ = s;
        direction_ = true;
        return 1;
    }
    send_buffer_len_ = 0;

    return 1;
}

int Shadowsocks::ShadowsocksHandleRemote()
{
    int r = recv(connect_socket_, recv_buffer_ + recv_buffer_len_, 
                1024 - recv_buffer_len_, 0);

    if(r == 0)
    {
        return -1;
    }
    else if(r == SOCKET_ERROR)
    {
        if(WSAGetLastError() == WSAEWOULDBLOCK)
        {
            return 1;
        }
        else
        {
            return -1;
        }
    }

    recv_buffer_len_ =+ r;

    int ret = crypto_->aead_decrypt(recv_buffer_, recv_buffer_len_, 
                                    send_buffer_, send_buffer_len_);
    
    if(ret == -1) return -1;
    
    if(send_buffer_len_ == 0)
    {
        recv_buffer_len_ = 0;
        return 1;
    }

    recv_buffer_len_ = 0;

    int s = send(accept_socket_, send_buffer_, send_buffer_len_, 0);
    if(s == -1)
    {
        if(WSAGetLastError() == WSAEWOULDBLOCK)
        {
            direction_ = false;
            return 1;
        }
        else
        {
            return -1;
        }
    }
    else if(s < send_buffer_len_)
    {
        send_buffer_idx_ = s;
        direction_ = false;
        return 1;
    }
    send_buffer_len_ = 0;

    return 1;
}


