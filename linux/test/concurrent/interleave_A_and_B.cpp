#include <mutex>
#include <iostream>
#include <thread>
#include <condition_variable>

using namespace std;

condition_variable cond;
mutex m;
bool print_A = true;

void print_char(char c, bool is_A)
{
    std::unique_lock<mutex> lock(m);
    cond.wait(lock, [is_A]{ return print_A == is_A; });
    print_A = !print_A;
    cout << c << endl;
    std::this_thread::sleep_for(500ms);
    lock.unlock();
    cond.notify_one();
}

void call_print_char(char c, bool is_A)
{
    while(true)
    {
        print_char(c, is_A);
    }
}

int main(void)
{
    thread t1(call_print_char, 'A', true);
    thread t2(call_print_char, 'B', false);
    t1.join();
    t2.join();

    return 0;
}