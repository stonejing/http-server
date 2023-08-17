#include "utils.h"
#include "log.h"
#include "threadpool.h"
#include <memory>

class Webserver
{
public:
    Webserver(int thread_num, int port);
    ~Webserver() {}

    void serverAcceptStart(); 
    void handleNewConnection(int accept_fd);

private:
    CLogger& Log = CLogger::getInstance();
    
    int listen_fd;
    int port;
    std::unique_ptr<ThreadPool> threadpool;
    bool quit = true;
};