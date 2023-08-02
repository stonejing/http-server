#include "winsock2.h"
#include "windows.h"
#include <iostream>
#include <vector>
#include <unordered_map>
#include <thread>
#include <mutex>

void thread_test()
{
    SOCKET dumb = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    std::cout << "generate dummy socket: " << dumb << std::endl;

    FD_SET read_set;

    while(true)
    {
        FD_ZERO(&read_set);
        FD_SET(dumb, &read_set);
        std::cout << "select in sub thread" << std::endl;
        int ret = select(0, &read_set, NULL, NULL, NULL);
        if(ret == SOCKET_ERROR) return;
        std::cout << "test." << std::endl;
    }
}

int main(void)
{
    WSADATA wsaData;

    if(WSAStartup(0x0202, &wsaData) != 0)
    {
        WSACleanup();
        std::cout << "none" << std::endl;
        return -1;
    }

    std::vector<int> test;

    std::cout << test.size() << std::endl;

    std::unordered_map<int, int> test_2;
    for(auto &a : test_2)
    {
        std::cout << "test_2" << std::endl;
    }


    int num = 3;

    std::thread t(thread_test);
    t.detach();

    while(num != 0)
    {
        std::cout << "test" << std::endl;
        num--;
    }
    std::cout << "main thread" << std::endl;
    SOCKET ttt = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    FD_SET main_set;
    FD_ZERO(&main_set);
    while(true)
    {
        FD_ZERO(&main_set);
        FD_SET(ttt, &main_set);
        std::cout << "select in main thread" << std::endl;
        int ret = select(0, &main_set, NULL, NULL, NULL);
        std::cout << "one loop" << std::endl;
    }
    
    return 1;
}