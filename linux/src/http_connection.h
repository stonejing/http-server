#pragma once

#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <assert.h>
#include <sys/stat.h>
#include <signal.h>
#include <string.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <stdarg.h>
#include <errno.h>
// #include "locker.h"
#include <sys/uio.h>
#include "channel.h"
#include <memory>
#include "dbg.h"
#include "event_loop.h"

class HttpConnection
{
public:
    /* 文件名的最大长度 */
    static const int FILENAME_LEN = 200;
    /* 读缓冲区的大小 */
    static const int READ_BUFFER_SIZE = 2048;
    /* 写缓冲区的大小 */
    static const int WRITE_BUFFER_SIZE = 1024;
    /* HTTP 请求方法，仅支持 GET */
    enum METHOD { GET = 0, POST, HEAD, PUT, DELETE,
                TRACE, OPTIONS, CONNECT, PATCH };
    /* 解析 HTTP 请求时，主状态机的状态*/
    enum CHECK_STATE { CHECK_STATE_REQUESTLINE = 0,
                        CHECK_STATE_HEADER,
                        CHECK_STATE_CONTENT };
    /* 服务器处理 HTTP 请求可能的结果 */
    enum HTTP_CODE { NO_REQUEST = 0, GET_REQUEST, BAD_REQUEST,
                    NO_RESOURCE, FORBIDDEN_REQUEST, FILE_REQUEST,
                    INTERNAL_ERROR, CLOSED_CONNECTION };
    /* HTTP 请求行的读取状态 */
    enum LINE_STATUS { LINE_OK = 0, LINE_BAD, LINE_OPEN };

public:
    /* 默认构造和析构函数 */
    HttpConnection(EventLoop* event_loop, int connfd);
    ~HttpConnection()
    {
        // log_info("close fd.");
        // close(m_sockfd);
    }

    std::shared_ptr<Channel> get_channel()
    {
        return channel_;
    }

private:
    void init();

    /* 处理客户请求，根据 HTTP request 内容，生成 response */
    void process();
    /* 非阻塞读，通过 socket 接受 request 内容*/
    bool read();
    /* 非阻塞写，通过 socket 发送 response 内容*/
    bool write();

    void HandleWrite();
    void HandleRead();

    void set_http_code(HTTP_CODE http_code)
    {
        http_code_ = http_code;
    }

    void CloseConnection();

    HTTP_CODE get_http_code()
    {
        return http_code_;
    }

    /* 解析 HTTP 请求*/
    HTTP_CODE ProcessRead();
    /* process_read 调用分析 HTTP 请求*/
    LINE_STATUS ParseLine();
    HTTP_CODE ParseRequestLine(char* text);
    HTTP_CODE ParseHeaders(char* text);
    HTTP_CODE ParseContent(char* text);
    /* 读取请求文件 */
    HTTP_CODE DoRequest();
    char* get_line() { return m_read_buf + m_start_line; }

    /* 填充 HTTP 应答*/
    bool ProcessWrite(HTTP_CODE ret);
    /* process_write 调用填充 HTTP 应答*/
    void unmap();
    bool AddResponse(const char* format, ...);
    bool AddContent(const char* content);
    bool AddStatusLine(int status, const char* title);
    bool AddHeaders(int content_length);
    bool AddContentLength(int content_length);
    bool AddLinger();
    bool AddBlankLine();

public:
    /* 用户数量 */
    // static int m_user_count

private:
    /* 该 HTTP 连接的 socket 和对方的 socket 地址*/
    int m_sockfd;
    /* 读缓冲区 */
    char m_read_buf[READ_BUFFER_SIZE];
    /* 缓冲区中已读入的最后一个字节的下一个位置 */
    int m_read_idx;
    /* 当前正在分析的字符在读缓冲区中的位置 */
    int m_checked_idx;
    /* 当前正在解析的行的起始位置 */
    int m_start_line;
    /* 写缓冲区 */
    char m_write_buf[WRITE_BUFFER_SIZE];
    /* 写缓冲区中待发送的字节数 */
    int m_write_idx;

    /* 状态机当前的状态 */
    CHECK_STATE m_check_state;
    /* HTTP 请求方法 */
    METHOD m_method;

    int bytes_to_send;
    int bytes_have_send;

    /* HTTP 请求文件，为 doc_root + m_url*/
    char m_read_file[FILENAME_LEN];
    /* HTTP 请求文件名 */
    char* m_url;
    /* HTTP 协议版本号，仅支持 HTTP/1.1 */
    char* m_version;
    /* 主机名 */
    char* m_host;
    /* HTTP 请求的消息体的长度 */
    int m_content_length;
    /* HTTP 请求是否要求保持连接 */
    bool m_linger;
    /* 目标文件 mmap 到内存中的起始位置 */
    char* m_file_address;
    /* 目标文件的状态。*/
    struct stat m_file_stat;
    /* 使用 writev 执行写操作，需要以下两个成员 */
    struct iovec m_iv[2];
    int m_iv_count;
    
    static int m_user_count;    /* 存储总的 http 连接数量 */

    HTTP_CODE http_code_;
    std::shared_ptr<Channel> channel_;
    EventLoop* loop_;
};