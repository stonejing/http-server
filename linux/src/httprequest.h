// a series functions that parse http header
// 分析 HTTP Request

#include <string>
#include <vector>
#include <map>

using namespace std;

enum METHOD { GET = 0, POST, HEAD, PUT,
              DELETE, TRACE, OPTIONS, CONNECT, PATCH };
enum CHECK_STATE { STATE_REQUEST_LINE = 0, 
                   STATE_HEADER,
                   STATE_CONTENT }; 
enum HTTP_CODE { NO_REQUEST = 0, GET_REQUEST, BAD_REQUEST, 
                 NO_RESOURCE, FORBIDDEN_REQUEST, FILE_REQUEST,
                 INTERNAL_ERROR, CLOSED_CONNECTION };
enum LINE_STATUS { LINE_OK  = 0, LINE_BAD, LINE_OPEN };


class HttpRequest
{
public:
    void add_buffer(string& inBuffer);
    // return true if the full http request is got, if true, reset http request variable
    void request_reset()
    {
        buffer_.clear();
        start_pos_ = 0;
        headers_.clear();
    }
    // 三种情况：完成 0，未完成 1，(错误 2，假定不会发生)
    int get_parse_status()
    {
        if(status_ == 0)
        {
            request_reset();
        }
        return status_;
    }        
    void get_information(bool keep_alive, string& URL);

private:
    // 假定所有的 HTTP request 的格式都是对的，没有出错的情况，只会有接收不完全的情况
    HTTP_CODE   parse_request_line();    // the first line of request
    HTTP_CODE   parse_headers();        // store all headers, only concentrate keep-alive
    HTTP_CODE   parse_content();        // only for head or post method
    
    string buffer_;                     // buffer that should be parsed

    int start_pos_       = 0;
    int end_pos_         = 0;

    map<string, string> headers_;

    string method_;                     // only support GET
    string http_version_;               // currently only support HTTP/1.1
    string URL_;
    bool keep_alive_ = false;
    int status_    = 1;
};