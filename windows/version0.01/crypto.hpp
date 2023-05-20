#ifndef _CRYPTO_H
#define _CRYPTO_H

#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/md5.h>
#include <openssl/kdf.h>

typedef struct buffer {
    size_t idx;
    int len;
    size_t capacity;
    uint8_t  *data;
} buffer_t;

// aead_aes_128_gcm
// key size     16
// salt size    16
// nonce size   12
// tag size     16
// EVP_BytesToKey   password ---> key
// HKDF_SHA1(key, salt, info) => subkey
typedef struct {
    uint32_t init;
    EVP_CIPHER_CTX *evp;
    buffer_t *chunk;
    uint8_t* salt;      // 16
    uint8_t* skey;      // 16
    uint8_t* nonce;     // 12
} cipher_ctx_t;

#endif