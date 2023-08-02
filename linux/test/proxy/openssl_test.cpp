#include <openssl/evp.h>
#include <mbedtls/md.h>
#include <openssl/md5.h>
#include <openssl/rand.h>
#include <string.h>

static uint8_t enc_key[32];
static int enc_key_len;
static int enc_iv_len;

int main()
{
    const char* pass = "stonejing";
    OpenSSL_add_all_algorithms();
    uint8_t iv[EVP_MAX_IV_LENGTH];
    const EVP_CIPHER *cipher = EVP_get_cipherbyname("aes-256-cfb");
    if (cipher == NULL)
    {
        perror("method no supported.");
        return 1;
    }

    enc_key_len = EVP_BytesToKey(cipher, EVP_md5(), NULL, (uint8_t *)pass,
                                 strlen(pass), 1, enc_key, iv);
    enc_iv_len = EVP_CIPHER_iv_length(cipher);
    printf("enc_key is: ");
    for(int i = 0; i < enc_key_len; i++)
    {
        printf("%d", enc_key[i]);
    }
    printf("\n");
    return 1;
}