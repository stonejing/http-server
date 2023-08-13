#include "httprequest.h"
#include <algorithm>

string METHOD[8] = {"GET", "POST", "HEAD", "PUT", "DELETE", "TRACE", "OPTIONS", "CONNECT"};

void HttpRequest::add_buffer(string&& inBuffer)
{
    buffer_.append(inBuffer);
}

void HttpRequest::http_parse()
{
    while(state_ != STATE_FINISH)
    {
        switch(state_)
        {
            case STATE_REQUEST_LINE:
            {
                HTTP_CODE ret = parse_request_line();
                if(ret == GET_REQUEST)
                {
                    state_ = STATE_HEADER;
                }
                else if(ret == NO_REQUEST)
                {
                    status_ = 0;
                    return;
                }
                else if(ret == BAD_REQUEST)
                {
                    status_ = -1;
                    return;
                }
                break;
            }
            case STATE_HEADER:
            {
                HTTP_CODE ret = parse_headers();
                if(ret == GET_REQUEST)
                {
                    state_ = STATE_CONTENT;
                }
                else if(ret == NO_REQUEST)
                {
                    status_ = 0;
                    return;
                }
                else if(ret == BAD_REQUEST)
                {
                    status_ = -1;
                    return;
                }
                break;
            }
            case STATE_CONTENT:
            {
                HTTP_CODE ret = parse_content();
                if(ret == GET_REQUEST)
                {
                    // check HTTP type, 1 http request, 2 http proxy, 3 https proxy    
                    status_ = 1;
                    if(headers_.count("host"))
                    {
                        string host = headers_["host"];
                        if(host != HOST)
                        {
                            status_ = 2;
                        }
                    }
                    else if(method_ == "CONNECT")
                    {
                        status_ = 3;
                    }
                    state_ = STATE_FINISH;
                }
                else if(ret == NO_REQUEST)
                {
                    status_ = 0;
                    return;
                }
                else if(ret == BAD_REQUEST)
                {
                    status_ = -1;
                    return;
                }
                break;
            }
            default:
            {
                status_ = -1;
                return;
            }
        }
    }
}

void HttpRequest::get_information(bool& keep_alive, string& URL, map<string, string>& headers)
{
    keep_alive = keep_alive_;
    URL = URL_;
    headers = headers_;
    request_reset();
}

HTTP_CODE HttpRequest::parse_request_line()
{
    // get HTTP method
    end_pos_ = buffer_.find(" ", start_pos_);
    if(end_pos_ == -1) return NO_REQUEST;
    method_ = buffer_.substr(start_pos_, end_pos_);
    if(std::find(METHOD, METHOD + 8, method_) == METHOD + 8) return BAD_REQUEST;
    // get HTTP URL
    start_pos_ = end_pos_ + 1;
    end_pos_ = buffer_.find(" ", start_pos_);
    if(end_pos_ == -1) return NO_REQUEST;
    URL_ = buffer_.substr(start_pos_, end_pos_ - start_pos_);
    //get HTTP version
    start_pos_ = end_pos_ + 1;
    end_pos_ = buffer_.find("\r\n", start_pos_);
    if(end_pos_ == -1) return NO_REQUEST;
    string version = buffer_.substr(start_pos_, end_pos_ - start_pos_);
    if(version != "HTTP/1.1") return BAD_REQUEST;
    start_pos_ = end_pos_ + 2;

    return GET_REQUEST;    
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
        if(buffer_.substr(end_pos_, 4) == "\r\n\r\n") break;
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

    return GET_REQUEST;
}

HTTP_CODE HttpRequest::parse_content()
{
    if(headers_.count("content-length"))
    {
        int content_length = stoi(headers_["content-length"]);

        if(buffer_.size() - start_pos_ + 1 == content_length)
            return GET_REQUEST;
        else 
            return NO_REQUEST;
    }
    else
    {
        return GET_REQUEST;
    }
}