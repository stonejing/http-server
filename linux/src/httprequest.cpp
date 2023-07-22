#include "httprequest.h"
#include <algorithm>
#include <cctype>

HTTP_CODE HttpRequest::parse_request_line()
{
    // get http method
    end_pos = buffer.find(" ", start_pos);
    if(end_pos == -1) return BAD_REQUEST;
    string method_request = buffer.substr(start_pos, end_pos);
    // get http URL
    start_pos = end_pos + 1;
    end_pos = buffer.find(" ", start_pos);
    if(end_pos == -1) return BAD_REQUEST;
    URL = buffer.substr(start_pos, end_pos - start_pos);
    //get http version
    start_pos = end_pos + 1;
    end_pos = buffer.find("\r\n", start_pos);
    if(end_pos == -1) return BAD_REQUEST;
    string version = buffer.substr(start_pos, end_pos - start_pos);
    start_pos = end_pos + 2;

    return NO_REQUEST;    
}

METHOD parse_method()
{
    return GET;
}

HTTP_CODE HttpRequest::parse_headers()
{
    while(end_pos != -1)
    {
        end_pos = buffer.find(": ", start_pos);
        if(end_pos == -1) break;
        string key = buffer.substr(start_pos, end_pos - start_pos);
        std::transform(key.begin(), key.end(), key.begin(), ::tolower);
        start_pos = end_pos + 2;
        end_pos = buffer.find("\r\n", start_pos);
        string value = buffer.substr(start_pos, end_pos - start_pos);
        std::transform(value.begin(), value.end(), value.begin(), ::tolower);
        start_pos = end_pos + 2;
        headers[key] = value;
    }

    if(headers.count("connection"))
    {
        if(headers["connection"] == "keep-alive")
            keep_alive = true;
    }

    return NO_REQUEST;
}

HTTP_CODE HttpRequest::parse_content()
{
    // 对于 GET 请求，完全没有必要，可以忽略
    // 对于 POST 请求是有必要的
    return NO_REQUEST;
}