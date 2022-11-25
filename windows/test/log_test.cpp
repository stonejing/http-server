#include "../src/log.h"
#include <thread>
#include <chrono>

void func1()
{
    Log& log = Log::get_instance();

    for(int i = 0; i < 10; i++)
    {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        log.PrintNum();
        log << "test";
    }
}

int main()
{
    // Log& log1 = Log::get_instance();
    // Log& log2 = Log::get_instance();

    // log1.PrintNum();
    // log2.PrintNum();

    LOG_WARN << "test" << "\n";
    LOG_INFO << "INFO\n";
    LOG_ERROR << "I do not know\n";

    // std::thread t1(func1);
    // // std::thread t2(func1);

    // t1.join();
    // t2.join();

    return 0;
}