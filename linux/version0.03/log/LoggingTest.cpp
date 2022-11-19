#include "Logging.h"
#include <unistd.h>
#include <iostream>

using namespace std;

/*
    需要等待线程先创建好才行。但是这里并不是线程的问题。
*/

int main(void)
{
    LOG << "test";
    LOG << "test" << "NIHAO";
    LOG << 11 << " " << 3.3 << "测试.";
    LOG << (short) 1 << " " << string("This is a string");
    sleep(5);
    return 0;
}