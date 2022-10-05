#include "shadowsocks.h"

int Shadowsocks::ShadowsocksHandleLocal()
{
    int r = recv(accept_socket_, recv_buffer_ + recv_buffer_len_, 1024 - recv_buffer_len_, 0);
    if(r == 0)
    {
        return -1;
    }
    else if(r == 1)
    {
        if(WSAGetLastError() == EAGAIN || WSAGetLastError() == EWOULDBLOCK)
        {
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
        log_err("unknown happed stage: %d", stage_);
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
        log_err("unsupported command: %d", request->cmd);
        response.rep = SOCKS5_REP_CMD_NOT_SUPPORTED;
        char* send_buf = (char*)&response;
        send(accept_socket_, send_buf, 4, 0);
        return -1;
    }

    if(connect(connect_socket_, (SOCKADDR*)&remote_server_, sizeof(remote_server_)) < 0)
    {
        log_err("connect remote error %d", WSAGetLastError());
        return -1;
    }

    ULONG NonBlock = 1;
    if(ioctlsocket(connect_socket_, FIONBIO, &NonBlock) == SOCKET_ERROR)
    {
        log_err("can not set socket to nonblock %d", WSAGetLastError());
    }

    char ciphertext[2048];
    int ciphertext_len;

    if(crypto_->aead_encrypt(recv_buffer_ + 3, recv_buffer_len_ - 3, 
                            ciphertext, ciphertext_len) == -1)
    {
        log_err("connect encryption error.");
        return -1;
    }

    send(connect_socket_, ciphertext, ciphertext_len, 0);

    recv_buffer_[1] = SOCKS5_REP_SUCCEEDED;

    send(accept_socket_, recv_buffer_, recv_buffer_len_, 0);

    return 1;
}

int Shadowsocks::ShadowsocksStageStream()
{
    char ciphertext[2048];
    int ciphertext_len;
    if(crypto_->aead_encrypt(recv_buffer_, recv_buffer_len_, 
            ciphertext, ciphertext_len) == -1)
    {
        return -1;
    }
    int r = send(connect_socket_, ciphertext, ciphertext_len, 0);
    if(r != ciphertext_len) return -1;
    return 1;
}

int Shadowsocks::ShadowsocksHandleRemote()
{
    char plaintext[20 * 1024];
    int plaintext_len = 0;

    int r = recv(connect_socket_, recv_buffer_ + recv_buffer_len_, 1024 - recv_buffer_len_, 0);

    if(r <= 0)
    {
        if(WSAGetLastError() == EAGAIN || WSAGetLastError() == EWOULDBLOCK)
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
                                    plaintext, plaintext_len);

    if(plaintext_len == 0)
    {
        recv_buffer_len_ = 0;
        return 1;
    }

    send(accept_socket_, plaintext, plaintext_len, 0);
    recv_buffer_len_ = 0;
    return 1;
}
