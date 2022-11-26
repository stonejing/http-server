#include "http.h"
#include <WinSock2.h>
#include <string>
#include <winsock2.h>

const char* ok_200_title    = "OK";
const char* error_400_title = "Bad Request";
const char* error_400_form  = "Your request has bad syntax or \
                                is inherently impossible to satisfy.\n"; 
const char* error_403_title = "Forbidden";
const char* error_403_form  = "You do not have permission to get file from this server.\n";
const char* error_404_title = "Not founnd";
const char* error_404_form  = "The requested file was not found on this server.\n";
const char* error_500_title = "Internal Error";
const char* error_500_form  = "There was an unusual problem serving the requested file.\n";

const char* doc_root = "";

/*
    http process procedure
    read request data
    process read data
    process write data
    write response data
    buffer choice: raw char* array > vector<char> > string
    vector<char> is better I think, and therr should have a class for buffer class
    a class will reduce speed, perhaps, zeros-cose-abstraction?
    how to determine the impact of compiler as it is more and more intelligent.
*/
HttpConnection::HttpConnection(SOCKET sockfd)
 : sockfd(sockfd), m_read_idx(0)
{

}

HttpConnection::HttpConnection(SOCKET sockfd, vector<char>& first_buffer, int first_buffer_len)
 : sockfd(sockfd)
{
    read_buffer_ = first_buffer;
    read_buffer_len_ = first_buffer_len;
}

HttpConnection::~HttpConnection()
{
    closesocket(sockfd);
}

void HttpConnection::Init()
{
    m_check_state = CHECK_STATE_REQUESTLINE;
    m_linger = false;

    m_method = GET;
    m_url = 0;
    m_version = 0;
    m_content_length = 0;
    m_host = 0;
    m_start_line = 0;
    m_checked_idx = 0;
    m_write_idx = 0;
    memset(m_read_buf, 0, READ_BUFFER_SIZE);
    memset(m_write_buf, 0, WRITE_BUFFER_SIZE);
    memset(m_file_path, 0, FILENAME_LEN);
}

HttpConnection::LINE_STATUS HttpConnection::ParseLine()
{
    char temp;
    for(; m_checked_idx < m_read_idx; ++m_checked_idx)
    {
        temp = m_read_buf[m_read_idx];
        if(temp == '\r')
        {
            if((m_checked_idx + 1) == m_read_idx)
                return LINE_OPEN;
            else if(m_read_buf[m_checked_idx + 1] == '\n')
            {
                m_read_buf[m_checked_idx++] = '\0';
                m_read_buf[m_checked_idx++] = '\0';
                return LINE_OK;
            }
            return LINE_BAD;
        }
        else if(temp == '\n')
        {
            if((m_checked_idx > 1) && (m_read_buf[m_checked_idx - 1] == '\r'))
            {
                m_read_buf[m_checked_idx - 1] = '\0';
                m_read_buf[m_checked_idx++]   = '\0';
                return LINE_OK;
            }
            return LINE_BAD;
        }
    }
    return LINE_BAD;
}

bool HttpConnection::Read()
{
    read_buffer_len_ = recvn(sockfd, read_buffer_);
    if(read_buffer_len_ == -1) return false;
    return true;
}

HttpConnection::HTTP_CODE HttpConnection::ParseRequestLine(char* text)
{
    return NO_REQUEST;
}

bool HttpConnection::Write()
{
    std::string result = "HTTP/1.1 200 OK\r\nContent-length: 17\r\n\r\nTHIS IS A TEST.\r\n";
    std::vector<char> http_response(result.begin(), result.end());
    sendn(sockfd, http_response, static_cast<int>(http_response.size()));
    return true;
}

int HttpConnection::Process()
{
    if(Read())
    {
        Write();
        return 1;
    }
    return -1;
}


