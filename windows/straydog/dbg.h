#ifndef __dbg__h_
#define __dgb_h_

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>

#ifndef NDEBUG
#define debug(M, ...)
#else
#define debug(M, ...) fprintf(stderr, "DEBUG %s:%d: ", M "\n", __FILE__, __LINE__, ##__VA_ARGS__)
#endif

#define clean_errno() (errno == 0 ? "None" : strerror(errno))

#define log_err(M, ...) fprintf(stderr, "\u001b[31m[ERROR]\u001b[0m (%s:%d: \u001b[35merrno:\u001b[0m %s) " M "\n", __FILE__, __LINE__, clean_errno(), ##__VA_ARGS__)

#define log_warn(M, ...) fprintf(stderr, "\u001b[33m[WARN]\u001b[0m (%s:%d \u001b[35merrno:\u001b[0m %s) " M "\n", __FILE__, __LINE__, clean_errno(), ##__VA_ARGS__) 

#define log_info(M, ...) fprintf(stderr, "\u001b[32m[INFO]\u001b[0m (%s:%d) " M "\n", __FILE__, __LINE__, ##__VA_ARGS__)

#define check(A, M, ...) if(!(A)) {log_err(M, ##__VA_ARGS__); errno = 0; goto error; }

#define check_mem(A) check((A), "Out of memory.")

#define check_debug(A, M, ...) if(!(A)) {debug(M, ##__VA_ARGS__); errno = 0; goto error; }

// static inline uint8_t *to_hexstring(const uint8_t * buf, uint64_t size)
// {
//     uint8_t *hexbuf = (uint8_t*)malloc(sizeof(uint8_t) * (2 * size + 1));
//     for(int i = 0; i < size; i++)
//     {
//         sprintf((char*)hexbuf + i * 2, "%02x", (int)buf[i]);
//     }
//     hexbuf[2 * size] = '\0';
//     return hexbuf;
// }

// static uint8_t *debug_hexstring(const char* name, const uint8_t * buf, uint64_t size)
// {
//     uint8_t *hexbuf = (uint8_t*)malloc(sizeof(uint8_t) * (2 * size + 1));
//     for(int i = 0; i < size; i++)
//     {
//         sprintf((char*)hexbuf + i * 2, "%02x", (int)buf[i]);
//     }
//     hexbuf[2 * size] = '\0';
//     printf("%s:\n%s\n", name, hexbuf);
//     return hexbuf;
// }

// static void debug_hex(const uint8_t *buf, size_t len)
// {
//     uint8_t *hexstring = to_hexstring(buf, len);
//     printf("%s\n", hexstring);
//     free(hexstring);
// }



#endif