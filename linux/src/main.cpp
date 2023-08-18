#include <chrono>
#include <string>
#include <memory>
#include <thread>
#include <functional>
#include "assert.h"

#include "log.h"
#include "webserver.h"
#include "threadpool.h"

using std::string;

int main(void)
{
    int thread_num = 1;
    
#ifdef Debug
    int port = 8000;
#else
    int port = 80;
#endif

    string log_path = "./webserver.log";

    CLogger& Log = CLogger::getInstance();
    Log.init(log_path);

    Webserver http_server(thread_num, port);
    std::thread t1(std::bind(&Webserver::serverAcceptStart, &http_server));
    t1.detach();
    LOG_INFO("main start http server");

    return 0;
}