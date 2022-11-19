#include <openssl/md5.h>
#include <openssl/rand.h>

#include "dbg.hpp"
#include "stream_encrypt.hpp"

// stream 加密，不用区分什么 block，

static int enc_key_len;
static int enc_iv_len;
static uint8_t enc_key[EVP_MAX_KEY_LENGTH];

void enc_key_init(const char* pass)
{
    OpenSSL_add_all_algorithms();

    uint8_t iv[EVP_MAX_IV_LENGTH];
    const EVP_CIPHER *cipher = EVP_get_cipherbyname("aes-256-cfb");
    if (cipher == NULL)
    {
        log_err("Cipher %s not found in OpenSSL library", "aes-256-cfb");
        return;
    }

    enc_key_len = EVP_BytesToKey(cipher, EVP_md5(), NULL, (uint8_t *)pass,
                                 strlen(pass), 1, enc_key, iv);
    enc_iv_len = EVP_CIPHER_iv_length(cipher);    
}

void enc_ctx_init(enc_ctx* ctx, int enc)
{
    EVP_CIPHER_CTX_init(ctx->evp);
    if (!EVP_CipherInit_ex(ctx->evp, EVP_aes_256_cfb(), NULL, NULL, NULL, enc))
    {
        log_err("Cannot initialize cipher %s", "aes-256-cfb");
        exit(EXIT_FAILURE);
    }
    if (!EVP_CIPHER_CTX_set_key_length(ctx->evp, enc_key_len))
    {
        EVP_CIPHER_CTX_cleanup(ctx->evp);
        log_err("Invalid key length: %d", enc_key_len);
        exit(EXIT_FAILURE);
    }
    EVP_CIPHER_CTX_set_padding(ctx->evp, 1);
}

int ss_encrypt(unsigned char *plaintext, int plaintext_len, unsigned char* ciphertext, enc_ctx *ctx)
{
    int iv_len = 0;
    int err = 0;
    int c_len;

    if (!ctx->init)
    {
        uint8_t iv[EVP_MAX_IV_LENGTH];
        iv_len = enc_iv_len;
        RAND_bytes(iv, iv_len);
        EVP_EncryptInit_ex(ctx->evp, NULL, NULL, enc_key, iv);
        memcpy(ciphertext, iv, iv_len);
        ctx->init = 1;
    }

    err = EVP_EncryptUpdate(ctx->evp, ciphertext + iv_len,
                                &c_len, (const uint8_t *)plaintext, plaintext_len);
    if (!err)
    {
        log_err("encrypt update error.");
        free(ciphertext);
        free(plaintext);
        return -1;
    }

    return iv_len + c_len;
}

int ss_decrypt(unsigned char *ciphertext, int ciphertext_len, unsigned char* plaintext, enc_ctx *ctx)
{
    int iv_len = 0;
    int err = 0;
    int p_len;

    if (!ctx->init)
    {
        uint8_t iv[EVP_MAX_IV_LENGTH];
        iv_len = enc_iv_len;
        memcpy(iv, ciphertext, iv_len);
        EVP_CipherInit_ex(ctx->evp, NULL, NULL, enc_key, iv, 0);
        ctx->init = 1;
    }

    err = EVP_DecryptUpdate(ctx->evp, (uint8_t*)plaintext, &p_len,
                            (const uint8_t*)(ciphertext + iv_len), ciphertext_len - iv_len);

    if (!err)
    {
        log_err("decrypt update error.");
        free(ciphertext);
        free(plaintext);
        return -1;
    }

    return p_len;
}

