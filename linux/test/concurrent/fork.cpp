#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>

class Test
{
public:
    Test()
    {
        for(int i = 0; i < 2; i++)
        {
            pid_t pid = fork();
            if(pid > 0)
            {
                printf("parent: %d\n", pid);
                continue;
            }
            else
            {
                printf("child: %d, pid: %d.\n", i, pid);
                idx = i;
                cur = pid;
                break;
            }
        }
    }

    void output()
    {
        printf("output: idx: %d, pid: %d\n", idx, cur);
    }

private:
    int a = 0;
    int b = 1;
    int idx = 0;
    pid_t cur;
};

int main(void)
{
    // int a = 1;
    // int b = 1;
    // for(int i = 0; i < 1; i++)
    // {
    //     pid_t pid = fork();
    //     if(pid > 0)
    //     {
    //         printf("parent.%d\n", a);
    //         continue;
    //     }
    //     else
    //     {
    //         b++;
    //         printf("child. %d\n", b);
    //         break;
    //     }
    // }

    auto t = new Test();
    t->output();
    return 0;
}