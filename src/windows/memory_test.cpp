#include <windows.h>
#include <stdio.h>
#include <stdlib.h>

struct test
{
    char buffer[5] = {'1', '2', '3', '4', '5'};
    long long name = 11111111;
    char b1[1024];
    char b2[4096];
} test1;

int main(void)
{
    while(1)
    {
        struct test n;
        // printf("%ld\n", n.name);
    }
    return 0;
}