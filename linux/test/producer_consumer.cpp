#include <iostream>
#include <thread>
#include <queue>
#include <vector>
#include <mutex>
#include <condition_variable>

// https://juejin.cn/s/c%2B%2B%20%E5%A4%9A%E7%BA%BF%E7%A8%8B%E7%94%9F%E4%BA%A7%E8%80%85%E6%B6%88%E8%B4%B9%E8%80%85
// https://github.com/forhappy/Cplusplus-Concurrency-In-Practice/blob/master/zh/chapter11-Application/11.1%20Producer-Consumer-solution.md


std::queue<int> buffer;     // 缓冲区队列
const int MAX_SIZE = 10;    // 缓冲区最大容量

std::mutex mtx;             // 互斥锁
std::condition_variable cv; // 条件变量

void producer()
{
    for(int i = 0; i < 20; i++)
    {
        std::unique_lock<std::mutex> lock(mtx);
        cv.wait(lock, []{ return buffer.size() < MAX_SIZE; });
        buffer.push(i);
        std::cout << "producer: " << i << std::endl;
        lock.unlock();
        cv.notify_one();
    }
}

void consumer()
{
    for(int i = 0; i < 20; i++)
    {
        std::unique_lock<std::mutex> lock(mtx);
        cv.wait(lock, []{ return buffer.size() > 0; });
        int data = buffer.front();
        buffer.pop();
        std::cout << "consumer: " << data << std::endl;
        lock.unlock();
        cv.notify_one();
    }
}

std::mutex mtx_letter;
std::condition_variable cv_letter;
bool printA = true;
const int NUM_ITERATIONS = 10;
void printletter(const std::string& letter, bool isA)
{
    for(int i = 0; i < NUM_ITERATIONS; i++)
    {
        std::unique_lock<std::mutex> lock(mtx_letter);
        cv.wait(lock, [isA]{ return printA == isA; });
        std::cout << letter << " " << std::this_thread::get_id() << std::endl;
        printA = !isA;
        lock.unlock();
        cv.notify_all();
    }
}

int main(void)
{
    std::thread producer_thread(producer);
    std::thread consumer_thread(consumer);
    producer_thread.join();
    consumer_thread.join();

    std::thread letter_A(printletter, "A", true);
    std::thread letter_B(printletter, "B", false);

    letter_A.join();
    letter_B.join();

    return 0;
}


