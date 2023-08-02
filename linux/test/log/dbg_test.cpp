#include "dbg.h"

#include <iostream>

using namespace std;

int main(void) 
{
    log_info("this is a test.");
    log_warn("this is warn");
    log_err("this is error.");
    debug("this is debug.");

    // std::cout << "\033[31;1;4mHello\033[0m" << std::endl;
    // printf("\033[31;1mHello\033[0m\n");
    // printf("\033[32;1mHello\033[0m\n");
    // printf("\033[33;1mHello\033[0m\n");
    // printf("\033[34;1mHello\033[0m\n");
    // printf("\033[35;1mHello\033[0m\n");
    // printf("\033[36;1mHello\033[0m\n");
    // printf("\033[37;1mHello\033[0m\n");
    int i, j, n;
    for (i = 0; i < 11; i++) 
    {
        for (j = 0; j < 10; j++) 
        {
            n = 10 * i + j;
            if (n > 108) break;
            printf("\033[%dm %3d\033[m", n, n);
        }
        printf("\n");
    }

    return 1;
}
