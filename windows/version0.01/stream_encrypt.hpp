#pragma once

#include <string.h>
// #include <algorithm>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include <openssl/evp.h>
#include "crypto.hpp"

#define min(a,b) (((a)<(b))?(a):(b))
#define max(a,b) (((a)>(b))?(a):(b))

#define BLOCK_SIZE 32

int ss_encrypt(unsigned char *plaintext, int plaintext_len, unsigned char* ciphertext, enc_ctx *ctx);
int ss_decrypt(unsigned char *ciphertext, int ciphertext_len, unsigned char* plaintext, enc_ctx *ctx);

void enc_ctx_init(enc_ctx* ctx, int enc);
int enc_init(const char* pass, const char* method);
void enc_key_init(const char* pass);
