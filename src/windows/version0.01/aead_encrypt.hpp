#ifndef _AEAD_ENCRYPT
#define _AEAD_ENCRYPT

#include "crypto.hpp"

void aead_encrypt(cipher_ctx_t *ctx, uint8_t *plaintext, int plaintext_len, uint8_t *ciphertext, int* ciphertext_len);

void aead_decrypt(cipher_ctx_t *ctx, uint8_t *ciphertext, int ciphertext_len, uint8_t *plaintext, int* plaintext_len);

# endif