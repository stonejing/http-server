#include "dbg.hpp"

#include <iostream>

using namespace std;

int main(void)
{
    log_info("\033[31;1;4mthis is a test.\033[0m");
    log_warn("this is warn");
    log_err("this is error.");
    debug("this is debug.");

    std::cout << "\033[31;1;4mHello\033[0m" << std::endl;
    
    printf("\033[31;1;4mHello\033[0m");

    return 1;
}