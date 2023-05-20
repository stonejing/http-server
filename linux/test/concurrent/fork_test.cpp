#include <unistd.h>
#include <stdio.h>

int main(void)
{
    int count = 0;

    pid_t pid;
    pid = fork();
    if(pid < 0)
        printf("error in fork.\n");
    else if(pid == 0)
    {
        printf("child id: %d\n", getpid());
        count++;
    }
    else {
        printf("father id: %d\n", getpid());
    }

    printf("result: %d\n", count);

    return 0;
}