#pragma once

#include <unordered_map>
#include <winsock2.h>
#include <windows.h>

#include <vector>
#include <string>

#include "log.h"
#include "utils.h"

using std::vector;
using std::unordered_map;
using std::string;

class HttpConnection
{
public:
    static const int FILENAME_LEN       = 200;
    static const int READ_BUFFER_SIZE   = 2048;
    static const int WRITE_BUFFER_SIZE  = 2048;
    enum METHOD 
    { 
        GET = 0, 
        POST, 
        HEAD, 
        PUT, 
        HTTP_DELETE, 
        TRACE, 
        OPTIONS, 
        CONNECT, 
        PATCH
    };
    enum CHECK_STATE 
    {
        CHECK_STATE_REQUESTLINE = 0,
        CHECK_STATE_HEADER,
        CHECK_STATE_CONTENT
    };
    enum HTTP_CODE 
    {
        NO_REQUEST,
        GET_REQUEST,
        BAD_REQUEST,
        NO_RESOURCE,
        FORBINDDEN_REQUEST,
        FILE_REQUEST,
        INTERNAL_ERROR,
        CLOSED_CONNECTION
    };
    enum LINE_STATUS 
    { 
        LINE_OK = 0, 
        LINE_BAD, 
        LINE_OPEN 
    };

    HttpConnection(SOCKET sockfd);
    HttpConnection(SOCKET sockfd, vector<char>& first_buffer, int first_buffer_len);
    ~HttpConnection();

public:
    void Init(SOCKET sockfd, const sockaddr_in& addr);
    void CloseConnection(bool real_close = true);
    int Process();
    bool Read();
    bool Write();

    SOCKET get_socket()
    {
        return sockfd;
    }

private:
    void Init();
    HTTP_CODE ProcessRead();
    bool ProcessWrite(HTTP_CODE ret);
    // parse http request used in ProcessRead() functionn
    HTTP_CODE ParseRequestLine(vector<char>& request);
    HTTP_CODE ParseHeaders(char* text);
    HTTP_CODE ParseContent(char* text);
    // store the file to memory address
    HTTP_CODE DoReuqest();
    // char* GetLine()  { };
    // split http request into lines based on CRLF
    LINE_STATUS ParseLine();
    // used in ProcessWrite() for writing response

private:
    SOCKET sockfd;
    struct sockaddr_in m_address;
    char m_read_buf[READ_BUFFER_SIZE];
    int m_read_idx;
    int m_checked_idx;
    int m_start_line;
    char m_write_buf[WRITE_BUFFER_SIZE];
    int m_write_idx;

    CHECK_STATE m_check_state;

    char m_file_path[FILENAME_LEN];
    char* m_url;
    char* m_version;
    char* m_host;
    int m_content_length;
    bool m_linger;

    char* m_file_address;
    struct stat m_file_stat;

    vector<char> read_buffer_;
    int read_buffer_len_;
    std::unordered_map<string, string> http_headers_;
    METHOD method_;
};