#include <iostream>
#include <thread>
#include <string>

void foo()
{
    std::cout << "in foo \n";
}

void bar(std::string& a, std::string& b, std::string& c) 
{
    std::cout << "string a: " << a << std::endl;
    std::cout << "string b: " << b << std::endl;
    std::cout << "string c: " << c << std::endl;
}

int main() 
{
    std::thread first(foo);
    std::string a = "a";
    std::thread second(bar, std::ref(a), std::ref(a), std::ref(a));

    first.join();
    second.join();

    std::cout << "end of main\n";

    return 0;
}