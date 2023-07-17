#include <thread>
#include <vector>
#include <memory>
#include <iostream>
#include <mutex>

using namespace std;

class EventLoop
{
public:
    ~EventLoop()
    {
        cout << "Event Loop Destruct." << endl;
    }
    void Test()
    {
        cout << "EventLoop TEST" << endl;
    }

    void Run()
    {
        cout << "EventLoop Run" << endl;
    }
};

class ThreadPool
{
public:
    ThreadPool(int thread_numbers = 4)
    {
        for(int i = 0; i < thread_numbers; i++)
        {
            threads.emplace_back([this]{this->start();});
            threads[i].detach();
        }
        cout << "Thread Pool Created." << endl;
    }
    ~ThreadPool() 
    {
        cout << "Thread Pool Destructor" << endl;
        // for(auto& worker : threads)
        // {
        //     worker.join();
        // }
        for(auto& loop : loops)
        {
            delete loop;
        }
        cout << "A: " << a << endl;
    }

    EventLoop* getLoop(int a)
    {
        return loops[a];
    }

    bool status()
    {
        return false;
    }

    int size()
    {
        return loops.size();
    }

private:
    void start()
    {
        std::this_thread::sleep_for(100ms);
        auto loop = new EventLoop();
        loop->Test();
        std::unique_lock<mutex> lock(mut);
        loops.push_back(loop);
        a++;
        lock.unlock();
    }

private:
    std::vector<std::thread> threads;
    std::vector<EventLoop*> loops;
    std::mutex mut;
    int thread_numbers;
    int a = 0;
};

class MyTask {
public:
    void start() {
        std::cout << "Task executed by thread " << std::this_thread::get_id() << std::endl;
    }
};

int main() 
{
    const int numThreads = 8;
    std::vector<std::thread> threads;
    MyTask task;

    for (int i = 0; i < numThreads; ++i) {
        threads.emplace_back(&MyTask::start, &task);
        threads.back().detach();
    }

    // Sleep for some time to allow the detached threads to execute

    // unique_ptr<ThreadPool> c(make_unique<ThreadPool>(200));
    auto c = new ThreadPool(200);
    // wait for thread create; 不能在一开始就直接调用，需要判断 thread 有没有创建完成
    std::this_thread::sleep_for(std::chrono::seconds(2));
    auto l = c->getLoop(2);
    l->Run();
    delete c;
    return 0;
}
