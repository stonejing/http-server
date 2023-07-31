#include "encrypt.h"
#include <openssl/md5.h>
#include <openssl/rand.h>

#define max(a,b) (((a)>(b))?(a):(b))

static int enc_key_len;
static int enc_iv_len;
static uint8_t enc_key[32];

/*
    只需要两个 ctx
    一个加密，一个解密
*/

char* ss_encrypt(int buf_size, char *plaintext, ssize_t *len, struct enc_ctx *ctx)
{
    int c_len = *len + BLOCK_SIZE;
    int iv_len = 0;
    int err = 0;
    char *ciphertext = (char*)malloc(max(iv_len + c_len, buf_size));

    /*
        每一个连接，都新建了一个 iv，每一个连接都是不同的。
        这完全没有必要。因为 key 已经是不一样的了，而且为什么还需要一个 iv 呢？
        已经是流式加密了，加一个固定长度的 iv。
        完全不明白。完全不明白。完全不明白。
    */
    if (!ctx->init)
    {
        uint8_t iv[EVP_MAX_IV_LENGTH];
        iv_len = enc_iv_len;
        RAND_bytes(iv, iv_len);
        EVP_CipherInit_ex(ctx->evp, NULL, NULL, enc_key, iv, 1);
        memcpy(ciphertext, iv, iv_len);
        ctx->init = 1;
    }

    err = EVP_EncryptUpdate(ctx->evp, (uint8_t*)(ciphertext+iv_len),
                            &c_len, (const uint8_t *)plaintext, *len);
    if (!err)
    {
        free(ciphertext);
        free(plaintext);
        return NULL;
    }

    *len = iv_len + c_len;
    free(plaintext);
    return ciphertext;
}

char* ss_decrypt(int buf_size, char *ciphertext, ssize_t *len, struct enc_ctx *ctx)
{

    int p_len = *len + BLOCK_SIZE;
    int iv_len = 0;
    int err = 0;
    char *plaintext = (char*)malloc(max(p_len, buf_size));

    /*
        依旧需要，每一个连接设置一个 ctx 上下文。真的是没有必要的事情。
        我的链表那里，就错的一塌糊涂，老是出现内存错误。
    */
    if (!ctx->init)
    {
        uint8_t iv[EVP_MAX_IV_LENGTH];
        iv_len = enc_iv_len;
        memcpy(iv, ciphertext, iv_len);
        EVP_CipherInit_ex(ctx->evp, NULL, NULL, enc_key, iv, 0);
        ctx->init = 1;
    }

    err = EVP_DecryptUpdate(ctx->evp, (uint8_t*)plaintext, &p_len,
                            (const uint8_t*)(ciphertext + iv_len), *len - iv_len);

    if (!err)
    {
        free(ciphertext);
        free(plaintext);
        return NULL;
    }

    *len = p_len;
    free(ciphertext);
    return plaintext;
}

void enc_ctx_init(struct enc_ctx* ctx, int enc, const char* method)
{
    const EVP_CIPHER *cipher = EVP_get_cipherbyname(method);
    if (cipher == NULL)
    {
        printf("Cipher %s not found in OpenSSL library", method);
    }
    memset(ctx, 0, sizeof(struct enc_ctx));
    ctx->evp = EVP_CIPHER_CTX_new();

    if (!EVP_CipherInit_ex(ctx->evp, cipher, NULL, NULL, NULL, enc))
    {
        printf("Cannot initialize cipher %s", method);
        exit(EXIT_FAILURE);
    }
    if (!EVP_CIPHER_CTX_set_key_length(ctx->evp, enc_key_len))
    {
        EVP_CIPHER_CTX_free(ctx->evp);
        printf("Invalid key length: %d", enc_key_len);
        exit(EXIT_FAILURE);
    }
    EVP_CIPHER_CTX_set_padding(ctx->evp, enc_key_len);
}

void enc_key_init(const char *password, const char* method)
{
    OpenSSL_add_all_algorithms();
    uint8_t iv[EVP_MAX_IV_LENGTH];
    const EVP_CIPHER *cipher = EVP_get_cipherbyname(method);
    if (cipher == NULL)
    {
        perror("method no supported.");
        return;
    }
    enc_key_len = EVP_BytesToKey(cipher, EVP_md5(), NULL, (uint8_t *)password,
                                 strlen(password), 1, enc_key, iv);
    enc_iv_len = EVP_CIPHER_iv_length(cipher);
    printf("enc_key_len: %d\n", enc_key_len);
    printf("enc_iv_len: %d\n", enc_iv_len);
    printf("enc_key: ");
    for(int i = 0; i < enc_key_len; i++)
    {
        printf("%d", enc_key[i]);
    };
    printf("\n");
}
