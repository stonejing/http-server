#pragma once

#include <openssl/evp.h>
#include <openssl/md5.h>
#include <openssl/hmac.h>
#include <openssl/rand.h>
#include <openssl/sha.h>
#include <openssl/kdf.h>

#include "dbg.h"

#include <cstring>

class Crypto
{
public:
    Crypto();
    ~Crypto()
    {
        EVP_CIPHER_CTX_free(enc_ctx_);
        EVP_CIPHER_CTX_free(dec_ctx_);
    };

    int aead_encrypt(char* plaintext, int plaintext_len, 
                    char* ciphertext, int& ciphertext_len);
    int aead_decrypt(char* ciphertext, int ciphertext_len, 
                    char* plaintext, int& plaintext_len);

private:
    int rand_bytes(uint8_t* buf, int num);
    void evp_bytes_to_key(const uint8_t* input, size_t input_len,
                          uint8_t* key, size_t key_len);
    bool hkdf_sha1(const uint8_t *salt, size_t salt_len, const uint8_t *ikm,
                  size_t ikm_len, const uint8_t *info, size_t info_len,
                  uint8_t *okm, size_t okm_len);

    void enc_increase_nonce();
    void dec_increase_nonce();

    void encrypt_reset_iv();
    void decrypt_reset_iv();

    bool aead_chunk_encrypt(const uint8_t* plaintext, int plaintext_len,
                            uint8_t* tag,
                            uint8_t* ciphertext, int& ciphertext_len);
    bool aead_chunk_decrypt(const uint8_t* ciphertext, int ciphertext_len,
                            uint8_t* tag,
                            uint8_t* plaintext, int& plaintext_len);
private:
    EVP_CIPHER_CTX* enc_ctx_;
    EVP_CIPHER_CTX* dec_ctx_;

    bool encrypt_init_;
    bool decrypt_init_;

    uint8_t enc_nonce[12];
    uint8_t dec_nonce[12];

    uint8_t enc_subkey[32];
    uint8_t dec_subkey[32];

    uint8_t decrypt_buffer_[20 * 1024];
    int decrypt_buffer_len_;
    int payload_value_;
};