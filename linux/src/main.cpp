#include <chrono>
#include <string>
#include <memory>
#include <thread>
#include "assert.h"

#include "log.h"
#include "webserver.h"

#include "threadpool.h"

using std::string;

int main(void)
{
    int thread_num = 8;
    int port = 80;

    string log_path = "./webserver.log";

    CLogger& Log = CLogger::getInstance();
    Log.init(log_path);

    Webserver http_server(1, 8000);
    std::thread t1(std::bind(&Webserver::serverAcceptStart, &http_server));
    t1.detach();
    LOG_INFO("main start http server");
    // http_server.serverAcceptStart();

    Webserver http_proxy_server(1, 8001);
    std::thread t2(std::bind(&Webserver::serverAcceptStart, &http_proxy_server));
    t2.detach();
    LOG_INFO("main start http proxy server");
    return 0;
}