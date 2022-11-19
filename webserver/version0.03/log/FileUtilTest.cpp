#include "FileUtil.h"
#include <cstring>
#include <stdlib.h>
#include <stdio.h>
#include <thread>

void writelog(FileUtil& file, std::string& a)
{
    const char* logline = "this is a test.\n";
    for(int i = 0; i < 15; i++)
    {
        file.WriteToBuffer(a.c_str(), 2);  
        file.WriteToBuffer(logline, strlen(logline));    
    }
    file.Flush();
}

int main(void)
{
    FileUtil file("test.log", 12);
    // writelog(file);
    std::string a = "1 ";
    std::string b = "2 ";
    std::thread thread1(writelog, std::ref(file), std::ref(a));
    std::thread thread2(writelog, std::ref(file), std::ref(b));
    thread1.join();
    thread2.join();

    return 0;
}