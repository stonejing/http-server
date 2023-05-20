#include "dbg.hpp"


uint8_t *to_hexstring(const uint8_t * buf, uint64_t size)
{
    uint8_t *hexbuf = (uint8_t*)malloc(sizeof(uint8_t) * (2 * size + 1));
    for(int i = 0; i < size; i++)
    {
        sprintf((char*)hexbuf + i * 2, "%02x", (int)buf[i]);
    }
    hexbuf[2 * size] = '\0';
    return hexbuf;
}

uint8_t *debug_hexstring(const char* name, const uint8_t * buf, uint64_t size)
{
    uint8_t *hexbuf = (uint8_t*)malloc(sizeof(uint8_t) * (2 * size + 1));
    for(int i = 0; i < size; i++)
    {
        sprintf((char*)hexbuf + i * 2, "%02x", (int)buf[i]);
    }
    hexbuf[2 * size] = '\0';
    printf("%s:\n%s\n", name, hexbuf);
    return hexbuf;
}

void debug_hex(const uint8_t *buf, size_t len)
{
    uint8_t *hexstring = to_hexstring(buf, len);
    printf("%s\n", hexstring);
    free(hexstring);
}