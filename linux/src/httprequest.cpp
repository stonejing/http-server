#include "httprequest.h"
#include <algorithm>
#include <cctype>

void HttpRequest::add_buffer(string& inBuffer)
{
    buffer_ += inBuffer;
    if(start_pos_ == 0)
    {
        HTTP_CODE ret = parse_request_line();
        if(ret != NO_REQUEST)
        {
            status_ = 1;
            return;
        }
    }
    HTTP_CODE ret = parse_headers();
    if(ret == NO_REQUEST)
    {
        status_ = 0;
        return;
    }
}

void HttpRequest::get_information(bool keep_alive, string& URL)
{
    keep_alive_ = keep_alive;
    URL = URL_;
}

HTTP_CODE HttpRequest::parse_request_line()
{
    // get http method
    end_pos_ = buffer_.find(" ", start_pos_);
    if(end_pos_ == -1) return BAD_REQUEST;
    string method_request = buffer_.substr(start_pos_, end_pos_);
    // get http URL
    start_pos_ = end_pos_ + 1;
    end_pos_ = buffer_.find(" ", start_pos_);
    if(end_pos_ == -1) return BAD_REQUEST;
    URL_ = buffer_.substr(start_pos_, end_pos_ - start_pos_);
    //get http version
    start_pos_ = end_pos_ + 1;
    end_pos_ = buffer_.find("\r\n", start_pos_);
    if(end_pos_ == -1) return BAD_REQUEST;
    string version = buffer_.substr(start_pos_, end_pos_ - start_pos_);
    start_pos_ = end_pos_ + 2;

    return NO_REQUEST;    
}

HTTP_CODE HttpRequest::parse_headers()
{
    while(end_pos_ != -1)
    {
        end_pos_ = buffer_.find(": ", start_pos_);
        if(end_pos_ == -1) break;
        string key = buffer_.substr(start_pos_, end_pos_ - start_pos_);
        std::transform(key.begin(), key.end(), key.begin(), ::tolower);
        start_pos_ = end_pos_ + 2;
        end_pos_ = buffer_.find("\r\n", start_pos_);
        string value = buffer_.substr(start_pos_, end_pos_ - start_pos_);
        std::transform(value.begin(), value.end(), value.begin(), ::tolower);
        start_pos_ = end_pos_ + 2;
        headers_[key] = value;
    }

    if(headers_.count("connection"))
    {
        if(headers_["connection"] == "keep-alive")
            keep_alive_ = true;
    }

    return NO_REQUEST;
}

HTTP_CODE HttpRequest::parse_content()
{
    // 对于 GET 请求，完全没有必要，可以忽略
    // 对于 POST 请求是有必要的
    return NO_REQUEST;
}