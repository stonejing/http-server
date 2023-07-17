#include <chrono>
#include <string>
#include <memory>
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

    LOG_INFO("main start server");

    Webserver server(8, 8000);
    server.serverAcceptStart();

    return 0;
}