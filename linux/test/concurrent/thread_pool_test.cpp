#include <iostream>
#include <vector>
#include <chrono>

#include "thread_pool.hpp"

int test(int a, int b)
{
    std::cout << "hello " << "a: " << a << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(1));
    std::cout << "world " << "b: " << b << std::endl;
    return a * b;
}

int main()
{
    ThreadPool pool(5);
    std::vector<std::future<int>> results;

    std::function<int(int, int)> te = test;

    for(int i = 0; i < 8; ++i)
    {
        // results.emplace_back(
            pool.enqueue(test, i, i + 1);
        // );
    }



    // for(auto&& result : results) 
    //     std::cout << result.get() << ' ';
    // std::cout << std::endl;

    return 0;
}