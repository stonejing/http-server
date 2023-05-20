#include <string.h>
#include <stdlib.h>
#include <stdio.h>


int main(void)
{
    char* test1 = (char*)malloc(10);
    char* test2 = (char*)malloc(100);

    memcpy(test1, test2, 100);

    free(test1);
    free(test2);

    return 0;
}