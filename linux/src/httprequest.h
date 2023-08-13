#pragma once

#include <string>
#include <vector>
#include <map>
#include <iostream>

using namespace std;

enum CHECK_STATE { STATE_REQUEST_LINE = 0, 
                   STATE_HEADER,
                   STATE_CONTENT,
                   STATE_FINISH }; 
enum HTTP_CODE { NO_REQUEST = 0, GET_REQUEST, BAD_REQUEST, 
                 NO_RESOURCE, FORBIDDEN_REQUEST, FILE_REQUEST,
                 INTERNAL_ERROR, CLOSED_CONNECTION };
enum LINE_STATUS { LINE_OK  = 0, LINE_BAD, LINE_OPEN };

#ifdef Debug
    #define HOST "127.0.0.1:8000"
#else
    #define HOST "http.stonejing.link"
#endif

class HttpRequest
{
public:
    HttpRequest() = default;

    // append buffer to the end of the buffer_ and parse it
    void add_buffer(string&& inBuffer);
    void add_buffer(string& inBuffer)
    {
        add_buffer(std::move(inBuffer));
    }
    /*
       -1: parse error
        0: parse not finished
        1: parse finished, http request
        2: parse finished, http proxy
        3: parse finished, https proxy
    */
    int get_parse_status()
    {
        http_parse();
        return status_;
    }

    void get_information(bool& keep_alive, string& URL, map<string, string>& headers);

private:
    void        http_parse();           // set status_
    /*
        NO_REQUEST:     not a complete http request
        GET_REQUEST:    a complete http request
        BAD_REQUEST:    a bad http request
    */
    HTTP_CODE   parse_request_line();   // parse the first line of http request
    HTTP_CODE   parse_headers();        // parse headers and store it in headers_
    HTTP_CODE   parse_content();        // no need to parse content, just check the content length and content-length in headers
    
    /*
        if parse finished, reset the buffer, start_pos_ and headers_
    */
    void request_reset()
    {
        buffer_.clear();
        start_pos_ = 0;
        headers_.clear();
        state_ = STATE_REQUEST_LINE;
    }

    string buffer_{};                     // buffer that should be parsed

    int start_pos_       = 0;
    int end_pos_         = 0;

    map<string, string> headers_;

    string method_;                     // only support GET
    string http_version_;               // currently only support HTTP/1.1
    string URL_;                        // define resource location; the other purpose is set different callback
    bool keep_alive_ = false;           // whether keep alive
    // -1: parse error, 0: parse not finished, 1: parse finished, http request, 2: parse finished, http proxy, 3: parse finished, https proxy
    int status_    = -1;
    CHECK_STATE state_ = STATE_REQUEST_LINE;
};