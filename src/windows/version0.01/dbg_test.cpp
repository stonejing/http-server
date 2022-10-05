#include "dbg.hpp"

#include <iostream>
#include <fstream>

using namespace std;

int main(void)
{
    log_info("this is a test.");
    log_warn("this is warn");
    log_err("this is error.");
    debug("this is debug.");

    std::cout << "\033[31;1;4mHello\033[0m" << std::endl;
    
    printf("\033[31;1;4mHello\033[0m");

    ofstream myfile;
    myfile.open("example.txt");
    myfile << "\033[31;1;4Writing this to a file.\033[0m\n";
    myfile.close();

    return 0;
}