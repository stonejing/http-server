#include <iostream>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

int main(void)
{
    pid_t pid_1 = fork();
    printf("pid_1: %d\n", pid_1);
    pid_t pid_2 = fork();
    printf("pid_2: %d\n", pid_2);

    return 0;
}