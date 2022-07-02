#include <openssl/opensslv.h>
#include <openssl/crypto.h>
#include <openssl/evp.h>
#include "stdio.h"
#include "stdlib.h"
#include "string.h"

int main()
{
    // printf("version: %s", OPENSSL_info(3));
    unsigned char outbuf_encrypt[256];
    int outlen_encrpyt, tmplen_encypt;
    unsigned char outbuf_cipher[256];
    int outlen_cipher, tmplen_cipher;
    char* outfile = "result.txt";

    unsigned char key[] = {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15};
    unsigned char iv[] = {1,2,3,4,5,6,7,8};

    char intext[] = "Some Crypto Text, created by stonejing.";

    EVP_CIPHER_CTX* ctx_encrypt;
    EVP_CIPHER_CTX* ctx_decrypt;
    EVP_CIPHER_CTX* ctx_cipher_enc;
    EVP_CIPHER_CTX* ctx_cipher_dec;

    ctx_encrypt = EVP_CIPHER_CTX_new();
    ctx_decrypt = EVP_CIPHER_CTX_new();
    ctx_cipher_enc = EVP_CIPHER_CTX_new();
    ctx_cipher_dec = EVP_CIPHER_CTX_new();

    EVP_CipherInit_ex(ctx_cipher_enc, EVP_aes_256_cfb(), NULL, key, iv, 1);
    EVP_CipherInit_ex(ctx_cipher_dec, EVP_aes_256_cfb(), NULL, key, iv, 0);
    EVP_EncryptInit_ex(ctx_encrypt, EVP_aes_256_cfb(), NULL, key, iv);
    EVP_DecryptInit_ex(ctx_decrypt, EVP_aes_256_cfb(), NULL, key, iv);

    EVP_CIPHER_CTX_set_padding(ctx_cipher_enc, 1);
    EVP_CIPHER_CTX_set_padding(ctx_cipher_dec, 1);
    EVP_CIPHER_CTX_set_padding(ctx_encrypt, 1);
    EVP_CIPHER_CTX_set_padding(ctx_decrypt, 1);

    if (!EVP_EncryptUpdate(ctx_cipher_enc, outbuf_encrypt, &outlen_encrpyt, (unsigned char*)intext, strlen(intext))) {
        /* Error */
        EVP_CIPHER_CTX_free(ctx_cipher_enc);
        return 0;
    }

    // if (!EVP_EncryptFinal_ex(ctx_cipher_enc, outbuf_encrypt + outlen_encrpyt, &tmplen_encypt)) {
    //     /* Error */
    //     EVP_CIPHER_CTX_free(ctx_cipher_enc);
    //     return 0;
    // }
    // outlen_encrpyt += tmplen_encypt;
    
    EVP_CIPHER_CTX_free(ctx_cipher_enc);

    FILE* out = fopen(outfile, "wb");
    if (out == NULL) {
        /* Error */
        return 0;
    }
    fwrite(outbuf_encrypt, 1, outlen_encrpyt, out);
    fclose(out);
    return 1;

    return 0;
}