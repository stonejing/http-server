#include "log.h"

// CLogger& log = CLogger::getInstance();

#define LOG_INFO(msg, ...) Log.log_info(__FILE__, __LINE__, msg, ##__VA_ARGS__)

void thread_t()
{
    CLogger& Log = CLogger::getInstance();
    for(int i = 0; i < 2; i++)
    {
        LOG_INFO("TEST %d", i);
        cout << std::hash<std::thread::id>{}(std::this_thread::get_id()) << endl;
    }
}

int main(void)
{
    // CLogger& Log = CLogger::getInstance();
    // LOG_INFO("TEST %d", 5);
    // LOG_INFO("TEST %d", 5);
    thread t[10];
    for(int i = 0; i < 10; i++)
    {
        t[i] = thread(thread_t);
    }

    for(int i = 0; i < 10; i++)
    {
        t[i].join();
    }

    return 0;
}