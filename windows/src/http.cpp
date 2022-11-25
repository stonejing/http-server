#include "http.h"
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
 : sockfd(sockfd)
{

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
    if(m_read_idx >= READ_BUFFER_SIZE)
        return false;
    int bytes_read = 0;
    while(true)
    {
        bytes_read = recv(sockfd, m_read_buf + m_read_idx, READ_BUFFER_SIZE - m_read_idx, 0);
        if(bytes_read == 0)
        {
            return false;
        }
        else if(bytes_read == SOCKET_ERROR)
        {
            if(WSAGetLastError() == WSAEWOULDBLOCK)
            {
                LOG_INFO << "handle local WSAEWOULDBLOCK." << "\n";
                return true;
            }
            return false;
        }
        m_read_idx += bytes_read;
    }
    return true;
}

HttpConnection::HTTP_CODE HttpConnection::ParseRequestLine(char* text)
{
    return NO_REQUEST;
}

bool HttpConnection::Write()
{
    std::string status_line = "HTTP/1.1 200 OK";
    std::string blank_line = "\r\n";
    std::string content = "THIS IS A TEST";
    std::string content_length = "Content-length: " + std::to_string(content.length());
    std::string result = status_line + blank_line + content_length + blank_line + blank_line+ content;
    send(sockfd, result.c_str(), (int)result.length(), 0);
    return true;
}

void HttpConnection::Process()
{
    if(Read())
        Write();
}


