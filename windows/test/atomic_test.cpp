#include <atomic>
#include <thread>
#include <mutex>


class AtomicTest
{
public:
    void Add()
    {
        test = !test;
    }
    void Subtract()
    {
        test = !test;
    }

private:
    std::atomic<bool> test;

};