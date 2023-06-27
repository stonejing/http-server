#include <stdio.h>

void byteorder()
{
    union {
        short value;
        char union_bytes[sizeof(short)];
    } test;

    test.value = 0x0102;
    if((test.union_bytes[0] == 1) && (test.union_bytes[1] == 2))
    {
        printf("Big Endian.\n");
    }
    else {
        printf("Little Endian.\n");
    }
}

int main(void)
{
    byteorder();
    return 0;
}