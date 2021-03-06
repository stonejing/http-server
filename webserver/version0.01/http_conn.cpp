#include "http_conn.h"
#include "dbg.h"

/* 定义 HTTP 响应的一些状态信息 */
const char* ok_200_title = "OK";
const char* error_400_title = "Bad Request";
const char* error_400_from = "Your request has bad syntax or is inherently impossible to satisfy.\n";
const char* error_403_title = "Forbidden";
const char* error_403_form = "You do not have permission to get file from this server.\n";
const char* error_404_title = "Not Found";
const char* error_404_form = "The request file was not found on this server.\n";
const char* error_500_title = "Internal Error";
const char* error_500_form = "There was an unusual problem serving the reqeustted file.\n";
/* 网站的根目录 */
const char* doc_root = "./resource";

void http_conn::init()
{
    m_check_state = CHECK_STATE_REQUESTLINE;
    m_linger = false;

    m_method = GET;

    bytes_have_send = 0;
    bytes_to_send = 0;

    m_url = 0;
    m_version = 0;
    m_content_length = 0;
    m_host = 0;
    m_start_line = 0;
    m_checked_idx = 0;
    m_read_idx = 0;
    m_write_idx = 0;
    memset(m_read_buf, 0, READ_BUFFER_SIZE);
    memset(m_write_buf, 0, WRITE_BUFFER_SIZE);
    memset(m_read_file, 0, FILENAME_LEN);

    
}

/* 
    从状态机，用于解析一行内容
    这是一个 屁 的状态机
    而且，这样不就遍历了两遍么，效率减少了
    正则也不行，过去笨重，也是用状态机来的
 */
http_conn::LINE_STATUS http_conn::parse_line()
{
    char temp;
    for(; m_checked_idx < m_read_idx; ++m_checked_idx)
    {
        /* 分析出一行 */
        /* 当前要分析的字节 */
        temp = m_read_buf[m_checked_idx];
        /* 如果当前字节是 '\r', 读到一个完整行*/
        if(temp == '\r')
        {
            if((m_checked_idx + 1) == m_read_idx)
            {
                return LINE_OPEN;
            }
            else if(m_read_buf[m_checked_idx + 1] == '\n')
            {
                m_read_buf[m_checked_idx++] = '\0';
                m_read_buf[m_checked_idx++] = '\0';
                return LINE_OK;
            }
            return LINE_BAD;
        }
        /*
            弄虚作假，基本不会到这里的。
            一定是现有 \r 再有 \n
            除非 http 请求头 有问题
        */
        else if(temp == '\n')
        {
            if((m_checked_idx > 1) && (m_read_buf[m_checked_idx - 1] == '\r'))
            {
                m_read_buf[m_checked_idx - 1] = '\0';
                m_read_buf[m_checked_idx ++] = '\0';
                log_info("LINE OK.");
                return LINE_OK;
            }
            return LINE_BAD;
        }
    }
    return LINE_OPEN;
}

/* 
    分析 HTTP 请求的入口函数，HTTP 的解析是按照 行 进行
    这个可以算是一个 状态机 
*/
http_conn::HTTP_CODE http_conn::process_read()
{
    LINE_STATUS line_status = LINE_OK;  /* 记录当前行的读取状态 */
    HTTP_CODE ret = NO_REQUEST;         /* 当前 HTTP 请求处理结果 */
    char* text = 0;
    
    while(((m_check_state == CHECK_STATE_CONTENT) && (line_status == LINE_OK)) 
                || ((line_status = parse_line()) == LINE_OK))
    {
        text = get_line();
        m_start_line = m_checked_idx;
        // log_info("got 1 http line: %s.", text);
        
        switch(m_check_state)
        {
            /* 分析请求行 */
            case CHECK_STATE_REQUESTLINE:
            {
                ret = parse_request_line(text);
                if(ret == BAD_REQUEST)
                {
                    log_err("BAD REQUEST.");
                    return BAD_REQUEST;
                }
                break;
            }
            /* 分析头部字段 */
            case CHECK_STATE_HEADER:
            {
                ret = parse_headers(text);
                if(ret == BAD_REQUEST)
                {
                    log_err("BAD REQUEST.");
                    return BAD_REQUEST;
                }
                else if(ret == GET_REQUEST)
                {
                    log_info("GET REQUEST. DO REQUEST.");
                    return do_request();
                }
                break;
            }
            /* 分析内容段 */
            case CHECK_STATE_CONTENT:
            {
                ret = parse_content(text);
                if(ret == GET_REQUEST)
                {
                    log_info("GET REQUEST. DO REQUEST.");
                    return do_request();
                }
                line_status = LINE_OPEN;
                break;
            }
            default:
            {
                return INTERNAL_ERROR;
            }
        }
    }
    return NO_REQUEST;
}

/* 解析 HTTP 请求行，获得请求方法，目标URL，HTTP 版本号 */
http_conn::HTTP_CODE http_conn::parse_request_line(char* text)
{
    log_info("parse request line.");
    m_url = strpbrk(text, " \t");
    if(!m_url)
    {
        return BAD_REQUEST;
    }
    *m_url++ = '\0';

    char* method = text;
    if(strcasecmp(method, "GET") == 0)
    {
        m_method = GET;
    }
    else
    {
        return BAD_REQUEST;
    }

    m_url += strspn(m_url, " \t");
    m_version = strpbrk(m_url, " \t");
    if(!m_version)
    {
        return BAD_REQUEST;
    }
    *m_version++ = '\0';
    m_version += strspn(m_version, " \t");
    if(strcasecmp(m_version, "HTTP/1.1") != 0)
    {
        return BAD_REQUEST;
    }
    if(strncasecmp(m_url, "http://", 7) == 0)
    {
        m_url += 7;
        m_url = strchr(m_url, '/');
    }
    if(!m_url || m_url[0] != '/')
    {
        return BAD_REQUEST;
    }

    log_info("m_method: %d.", m_method);
    log_info("m_url: %s", m_url);
    log_info("m_version: %s", m_version);

    m_check_state = CHECK_STATE_HEADER;
    return NO_REQUEST;
}

http_conn::HTTP_CODE http_conn::parse_headers(char* text)
{
    if(text[0] == '\0')
    {
        if(m_content_length != 0)
        {
            m_check_state = CHECK_STATE_CONTENT;
            return NO_REQUEST;
        }
        return GET_REQUEST;
    }
    else if(strncasecmp(text, "Connection:", 11) == 0)
    {
        text += 11;
        text += strspn(text, " \t");
        if(strcasecmp(text, "keep-alive") == 0)
        {
            m_linger = true;
        }
    }
    else if(strncasecmp(text, "Content-Length:", 15) == 0)
    {
        text += 15;
        text += strspn(text, " \t");
        m_content_length = atol(text);
    }
    else if(strncasecmp(text, "Host:", 5) == 0)
    {
        text += 5;
        text += strspn(text, " \t");
        m_host = text;
    }
    else
    {
        log_info("oops! unknown header %s.", text);
        return NO_REQUEST;
    }
    return NO_REQUEST;
}

http_conn::HTTP_CODE http_conn::parse_content(char* text)
{
    if(m_read_idx >= (m_content_length + m_checked_idx))
    {
        text[m_content_length] = '\0';
        return GET_REQUEST;
    }

    return NO_REQUEST;
}

http_conn::HTTP_CODE http_conn::do_request()
{
    strcpy(m_read_file, doc_root);
    int len = strlen(doc_root);
    strncpy(m_read_file + len, m_url, FILENAME_LEN - len - 1);
    log_info("m_read_file: %s, m_url: %s.", m_read_file, m_url);
    if(stat(m_read_file, &m_file_stat) < 0)
    {
        log_err("NO RESOURCE.");
        return NO_RESOURCE;
    }
    if(!(m_file_stat.st_mode & S_IROTH))
    {
        log_err("FORBIDDEN REQUEST.");
        return FORBIDDEN_REQUEST;
    }
    if(S_ISDIR(m_file_stat.st_mode))
    {
        log_err("BAD REQUEST.");
        return BAD_REQUEST;
    }

    int fd = open(m_read_file, O_RDONLY);
    m_file_address = (char*)mmap(0, m_file_stat.st_size, PROT_READ,
                    MAP_PRIVATE, fd, 0);
    close(fd);
    return FILE_REQUEST;
}

void http_conn::unmap()
{
    if(m_file_address)
    {
        munmap(m_file_address, m_file_stat.st_size);
        m_file_address = 0;
    }
}

bool http_conn::add_response(const char* format, ...)
{
    if(m_write_idx >= WRITE_BUFFER_SIZE)
    {
        return false;
    }
    va_list arg_list;
    va_start(arg_list, format);
    int len = vsnprintf(m_write_buf + m_write_idx, WRITE_BUFFER_SIZE - 1 - m_write_idx,
                    format, arg_list);
    if(len >= (WRITE_BUFFER_SIZE - 1 - m_write_idx))
    {
        return false;
    }
    m_write_idx += len;

    va_end(arg_list);
    return true;
}

bool http_conn::add_status_line(int status, const char* title)
{
    return add_response("%s %d %s\r\n", "HTTP/1.1", status, title);
}

bool http_conn::add_headers(int content_len)
{
    add_content_length(content_len);
    add_linger();
    add_blank_line();
    return true;
}

bool http_conn::add_content_length(int content_len)
{
    return add_response("Content-Length: %d\r\n", content_len);
}

bool http_conn::add_linger()
{
    return add_response("Connection: %s\r\n", (m_linger == true) ? "keep-alive" : "close");
}

bool http_conn::add_blank_line()
{
    return add_response("%s", "\r\n");
}

bool http_conn::add_content(const char* content)
{
    return add_response("%s", content);
}

bool http_conn::process_write(HTTP_CODE ret)
{
    switch(ret)
    {
        case INTERNAL_ERROR:
        {
            add_status_line(500, error_500_title);
            add_headers(strlen(error_500_form));
            if(!add_content(error_500_form))
            {
                return false;
            }
            break;
        }
        case BAD_REQUEST:
        {
            add_status_line(400, error_400_title);
            add_headers(strlen(error_400_from) - 1);
            if(!add_content(error_400_from))
            {
                return false;
            }
            break;
        }
        case NO_RESOURCE:
        {
            log_info("NO RESOURCE RESPONSE.");
            add_status_line(404, error_404_title);
            add_headers(strlen(error_404_form) - 1);
            if(!add_content(error_404_form))
            {
                return false;
            }
            break;
        }
        case FORBIDDEN_REQUEST:
        {
            add_status_line(403, error_403_title);
            add_headers(strlen(error_403_form));
            if(!add_content(error_403_form))
            {
                return false;
            }
            break;
        }
        case FILE_REQUEST:
        {
            log_info("FILE REQUEST WRITE.");
            add_status_line(200, ok_200_title);
            if(m_file_stat.st_size != 0)
            {
                add_headers(m_file_stat.st_size);
                m_iv[0].iov_base = m_write_buf;
                m_iv[0].iov_len = m_write_idx;
                m_iv[1].iov_base = m_file_address;
                m_iv[1].iov_len = m_file_stat.st_size;
                m_iv_count = 2;
                bytes_to_send = m_write_idx + m_file_stat.st_size;
                // log_info("write buf:\n%s", m_write_buf);
                // log_info("%s", m_file_address);
                return true;
            }
            else
            {
                add_status_line(200, ok_200_title);
                const char* ok_string = "<html><body>HELLO</body></html>";
                add_headers(strlen(ok_string));
                if(!add_content(ok_string))
                {
                    return false;
                }
            }
        }
        default:
        {
            return false;
        }
    }

    m_iv[0].iov_base = m_write_buf;
    m_iv[0].iov_len = m_write_idx;
    m_iv_count = 1;
    bytes_to_send = m_write_idx;
    return true;
}

void http_conn::process()
{
    HTTP_CODE read_ret = process_read();
    if(read_ret == NO_REQUEST)
    {
        // modfd(m_epollfd, m_sockfd, EPOLLIN);
        return;
    }
    bool write_ret = process_write(read_ret);
    // if(!write_ret)
    // {
    //     close_conn();
    // }

    // modfd(m_epollfd, m_sockfd, EPOLLOUT);
}

/* 循环读取客户数据，直到无数据可读或对方关闭连接 */
bool http_conn::read()
{
    if(m_read_idx >= READ_BUFFER_SIZE)
    {
        return false;
    }
    int bytes_read = 0;
    // log_info("read bytes from socket.");
    /* 将 HTTP 请求全部读入，存到 m_read_buf char数组中*/
    while(true)
    {
        bytes_read = recv(m_sockfd, m_read_buf + m_read_idx, READ_BUFFER_SIZE - m_read_idx, 0);
        if(bytes_read == -1)
        {
            if(errno == EAGAIN || errno == EWOULDBLOCK)
            {
                break;
            }
            return false;
        }
        else if(bytes_read == 0)
        {
            return false;
        }
        m_read_idx += bytes_read;
    }
    // log_info("read completed. %d", m_read_idx);
    // for(int i = 0; i < m_read_idx; i++)
    // {
    //     log_info("%02x ", (unsigned char)m_read_buf[i]);
    // }
    // m_read_idx = 0;
    return true;
}

bool http_conn::write()
{
    int temp = 0;

    if(bytes_to_send == 0)
    {
        // modfd(m_epollfd, m_sockfd, EPOLLIN);
        init();
        return true;
    }
    /*
        这个 while 只能将缓冲区写满，写满之后，就是阻塞 failure
        因此需要 EPOLLOUT，在缓冲区可写时，再写一次。
        主要时 LINUX TCP 包的大小吧，就是这一方面的东西
        TCP 虽然是流，但是 IP 是包呀。LINUX 设置了一个缓冲区
    */
    while(1)
    {
        temp = writev(m_sockfd, m_iv, m_iv_count);
        // temp = send(m_sockfd, (char*)&response, sizeof(response), 0);
        if(temp <= -1)
        {
            printf("write failure.\n");
            if(errno == EAGAIN)
            {
                // printf("change mode.\n");
                /*
                    写完出错，如果不设置为 EPOLLOUT
                    可以会变成 EPOLLIN，这个可能才是默认的
                */
                // modfd(m_epollfd, m_sockfd, EPOLLOUT);
                printf("errno EAGAIN.\n");
                return true;
            }
            log_info("error: %d", errno);
            unmap();
            return false;
        }        
        bytes_to_send -= temp;
        bytes_have_send += temp;
        log_info("bytes have send: %d", bytes_have_send);
        log_info("bytes to send: %d", bytes_to_send);
        if(bytes_have_send >= m_iv[0].iov_len)
        {
            m_iv[0].iov_len = 0;
            m_iv[1].iov_base = m_file_address + (bytes_have_send - m_write_idx);
            m_iv[1].iov_len = bytes_to_send;
        }
        else
        {
            m_iv[0].iov_base = m_write_buf + bytes_have_send;
            m_iv[0].iov_len = m_iv[0].iov_len - bytes_have_send;
        }

        if(bytes_to_send <= 0)
        {
            unmap();
            log_info("change epoll event to EPOLLIN.");
            // modfd(m_epollfd, m_sockfd, EPOLLIN);
            if(m_linger)
            {
                init();
                return false;
            }
            else
            {
                return false;
            }
        }
    }
}