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
    void addBuffer(string& inBuffer)
    {
        buffer += inBuffer;
    }

    HTTP_CODE   parse_request_line();    // the first line of request
    HTTP_CODE   parse_headers();
    HTTP_CODE   parse_content();

    bool get_keep_alive()
    {
        return keep_alive;
    }

private:
    METHOD parse_method();
    void parse_URL();
    
    int start_pos       = 0;
    int end_pos         = 0;
    CHECK_STATE         checkstate;
    int start_line      = 0;
    string buffer;

    string URL;
    map<string, string> headers;

    bool keep_alive = false;
};