#ifndef ENCRYPT_H
#define ENCRYPT_H

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <openssl/evp.h>

#define BLOCK_SIZE 32

struct enc_ctx
{
    uint8_t init;
    EVP_CIPHER_CTX* evp;
};

char* ss_encrypt(int buf_size, char *plaintext, ssize_t *len, struct enc_ctx *ctx);
char* ss_decrypt(int buf_size, char *ciphertext, ssize_t *len, struct enc_ctx *ctx);
void enc_ctx_init(struct enc_ctx *ctx, int enc, const char* method);
void enc_key_init(const char *password, const char* method);

#endif // ENCRYPT_H
