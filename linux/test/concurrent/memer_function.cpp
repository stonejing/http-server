#include <chrono>
#include <iostream>
#include <thread>
#include <mutex>
#include <vector>

class EventLoop
{
public:
    EventLoop() : a(0), b(0)
    {
        std::cout << "a: " << a << " b: " << b << std::endl;
    } 
    ~EventLoop() {}

    void loop()
    {
        std::this_thread::sleep_for(std::chrono::microseconds(100));
        for(int i = 0; i < 100000; i++)
        {
            a++;
        }
        for(int i = 0; i < 100000; i++)
        {
            a--;
        }
        std::cout << a << b << std::endl;
        std::cout << std::this_thread::get_id() << std::endl;
    }

private:
    int a;
    int b; 
};

class ThreadPool
{
public:
    ThreadPool()
    {
        for(int i = 0; i < 10; i++)
        {
            threads.emplace_back([this]{this->start();});
        }
    }

    ~ThreadPool()
    {
        for(auto& thread : threads)
        {
            thread.join();
        }
    }

    void start()
    {
        auto eventloop = new EventLoop();
        eventloop->loop();
    }
private:
    std::vector<std::thread> threads;
};

int main(void)
{
    // std::vector<std::thread> threads;
    // // auto eventloop = new EventLoop();
    // for(int i = 0; i < 3; i++)
    // {
    //     auto eventloop = new EventLoop();
    //     threads.emplace_back(&EventLoop::loop, eventloop);
    // }

    // for(auto& thread : threads)
    // {
    //     thread.join();
    // }
    ThreadPool threadpool;

    return 0;
}