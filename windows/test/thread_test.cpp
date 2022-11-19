#include <iostream>
#include <process.h>
#include <processthreadsapi.h>
#include <thread>
#include <string>
#include <chrono>
#include "processthreadsapi.h"
#include <iostream>

void foo()
{
    std::cout << "in foo \n";
    int c = 10;
    int t = 0;
    for(int i = 0; i < c; i++)
    {   
        t++;
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    std::cout << "thread quit" << std::endl;
    ExitThread(0);
}

void bar(std::string& a, std::string& b, std::string& c) 
{
    std::cout << "string a: " << a << std::endl;
    std::cout << "string b: " << b << std::endl;
    std::cout << "string c: " << c << std::endl;
    while(true)
    {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        std::cout << a << " " << b << " " << c << " " << std::endl;
    }
}

int main() 
{
    std::thread first(foo);
    std::string a = "a";
    std::thread second(bar, std::ref(a), std::ref(a), std::ref(a));

    first.detach();
    second.detach();

    std::cout << "end of main\n";
    system("pause");
    return 0;
} 