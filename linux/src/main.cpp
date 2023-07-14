#include "log.h"
#include <chrono>
#include <ratio>
#include <thread>
#include <random>

// CLogger& log = CLogger::getInstance();

void thread_t()
{
    CLogger& Log = CLogger::getInstance("log2.txt");
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dist(0, 1000);

    int sleep_duration = dist(gen);

    for(int i = 0; i < 2; i++)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(sleep_duration));
        LOG_INFO("%s, TEST %d", "stonejing", i);
        LOG_WARN("%s, TEST %d", "stonejing", i);
        LOG_ERR("%s, TEST %d", "stonejing", i);
    }
}

int main(void)
{
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