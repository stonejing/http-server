#include "crypto.hpp"
// #include "aead_encrypt.hpp"
#include <assert.h>
#include <mbedtls/cipher.h>
#include <mbedtls/md.h>
#include <mbedtls/md5.h>


uint8_t *to_hexstring(const uint8_t * buf, uint64_t size)
{
    uint8_t *hexbuf = (uint8_t*)malloc(sizeof(uint8_t) * (2 * size + 1));
    for(int i = 0; i < size; i++)
    {
        sprintf((char*)hexbuf + i * 2, "%02x", (int)buf[i]);
    }
    hexbuf[2 * size] = '\0';
    return hexbuf;
}

uint8_t *debug_hexstring(const char* name, const uint8_t * buf, uint64_t size)
{
    uint8_t *hexbuf = (uint8_t*)malloc(sizeof(uint8_t) * (2 * size + 1));
    for(int i = 0; i < size; i++)
    {
        sprintf((char*)hexbuf + i * 2, "%02x", (int)buf[i]);
    }
    hexbuf[2 * size] = '\0';
    printf("%s:\n%s\n", name, hexbuf);
    return hexbuf;
}

void debug_hex(const uint8_t *buf, size_t len)
{
    uint8_t *hexstring = to_hexstring(buf, len);
    printf("%s\n", hexstring);
    free(hexstring);
}

#define MAX_MD_SIZE MBEDTLS_MD_MAX_SIZE

typedef mbedtls_md_info_t digest_type_t;

int
crypto_derive_key(const char *pass, uint8_t *key, size_t key_len)
{
    size_t datal;
    datal = strlen((const char *)pass);

    const digest_type_t *md = mbedtls_md_info_from_string("MD5");
    if (md == NULL) {
        printf("MD5 Digest not found in crypto library");
        return -1;
    }

    mbedtls_md_context_t c;
    unsigned char md_buf[MAX_MD_SIZE];
    int addmd;
    unsigned int i, j, mds;

    mds = mbedtls_md_get_size(md);
    memset(&c, 0, sizeof(mbedtls_md_context_t));

    if (pass == NULL)
        return key_len;
    if (mbedtls_md_setup(&c, md, 0))
        return 0;

    for (j = 0, addmd = 0; j < key_len; addmd++) {
        mbedtls_md_starts(&c);
        if (addmd) {
            mbedtls_md_update(&c, md_buf, mds);
        }
        mbedtls_md_update(&c, (uint8_t *)pass, datal);
        mbedtls_md_finish(&c, &(md_buf[0]));

        for (i = 0; i < mds; i++, j++) {
            if (j >= key_len)
                break;
            key[j] = md_buf[i];
        }
    }

    mbedtls_md_free(&c);
    return key_len;
}

/* HKDF-Extract(salt, IKM) -> PRK */
int
crypto_hkdf_extract(const mbedtls_md_info_t *md, const unsigned char *salt,
                    int salt_len, const unsigned char *ikm, int ikm_len,
                    unsigned char *prk)
{
    int hash_len;
    unsigned char null_salt[MBEDTLS_MD_MAX_SIZE] = { '\0' };

    if (salt_len < 0) {
        return -1;
    }

    hash_len = mbedtls_md_get_size(md);

    if (salt == NULL) {
        salt     = null_salt;
        salt_len = hash_len;
    }

    return mbedtls_md_hmac(md, salt, salt_len, ikm, ikm_len, prk);
}

/* HKDF-Expand(PRK, info, L) -> OKM */
int
crypto_hkdf_expand(const mbedtls_md_info_t *md, const unsigned char *prk,
                   int prk_len, const unsigned char *info, int info_len,
                   unsigned char *okm, int okm_len)
{
    int hash_len;
    int N;
    int T_len = 0, where = 0, i, ret;
    mbedtls_md_context_t ctx;
    unsigned char T[MBEDTLS_MD_MAX_SIZE];

    if (info_len < 0 || okm_len < 0 || okm == NULL) {
        return -1;
    }

    hash_len = mbedtls_md_get_size(md);

    if (prk_len < hash_len) {
        return -1;
    }

    if (info == NULL) {
        info = (const unsigned char *)"";
    }

    N = okm_len / hash_len;

    if ((okm_len % hash_len) != 0) {
        N++;
    }

    if (N > 255) {
        return -1;
    }

    mbedtls_md_init(&ctx);

    if ((ret = mbedtls_md_setup(&ctx, md, 1)) != 0) {
        mbedtls_md_free(&ctx);
        return ret;
    }

    /* Section 2.3. */
    for (i = 1; i <= N; i++) {
        unsigned char c = i;

        ret = mbedtls_md_hmac_starts(&ctx, prk, prk_len) ||
              mbedtls_md_hmac_update(&ctx, T, T_len) ||
              mbedtls_md_hmac_update(&ctx, info, info_len) ||
              /* The constant concatenated to the end of each T(n) is a single
               * octet. */
              mbedtls_md_hmac_update(&ctx, &c, 1) ||
              mbedtls_md_hmac_finish(&ctx, T);

        if (ret != 0) {
            mbedtls_md_free(&ctx);
            return ret;
        }

        memcpy(okm + where, T, (i != N) ? hash_len : (okm_len - where));
        where += hash_len;
        T_len  = hash_len;
    }

    mbedtls_md_free(&ctx);

    return 0;
}

/* HKDF-Extract + HKDF-Expand */
int
crypto_hkdf(const unsigned char *salt,
            int salt_len, const unsigned char *ikm, int ikm_len,
            const unsigned char *info, int info_len, unsigned char *okm,
            int okm_len)
{
    unsigned char prk[MBEDTLS_MD_MAX_SIZE];

    const digest_type_t *md = mbedtls_md_info_from_string("SHA1");

    return crypto_hkdf_extract(md, salt, salt_len, ikm, ikm_len, prk) ||
           crypto_hkdf_expand(md, prk, mbedtls_md_get_size(md), info, info_len,
                              okm, okm_len);
}


void test_crypto_aead_encrypt()
{
	unsigned char key[16] = {
		1, 2, 3, 4, 5, 6, 7, 8, 1, 2, 3, 4, 5, 6, 7, 8
	};
	unsigned char iv[12] = { 0, 0, 3, 4, 5, 6, 7, 8, 1, 2, 3, 4 };
	// unsigned char plaintext[8] = {'p', 'a', 's', 's', 'w', 'o', 'r', 'd'};
    unsigned char plaintext[9] = "password";
	unsigned char result1[8] = { 0x0b, 0x3b, 0x1e, 0x3b,
				     0x6f, 0x5e, 0xa3, 0x0c };
	unsigned char tag1[16] = { 0xa5, 0x33, 0x26, 0xb6, 0x34, 0xa1,
				   0x17, 0xf8, 0x78, 0xdc, 0x09, 0x0e,
				   0x76, 0x93, 0x47, 0x5e };

    cipher_ctx_t* encryptor = (cipher_ctx_t*)malloc(sizeof(cipher_ctx_t));
    cipher_ctx_t* decryptor = (cipher_ctx_t*)malloc(sizeof(cipher_ctx_t));

    encryptor->evp = EVP_CIPHER_CTX_new();
    decryptor->evp = EVP_CIPHER_CTX_new();

    EVP_EncryptInit_ex(encryptor->evp, EVP_aes_128_gcm(), NULL, key, iv);
    EVP_DecryptInit_ex(decryptor->evp, EVP_aes_128_gcm(), NULL, key, iv);

    uint8_t *salt = (uint8_t*)malloc(16);
    rand_bytes(salt, 16);
    debug_hex(salt, 16);

    uint8_t *ikey = (uint8_t*)malloc(16);
    uint8_t *mkey = (uint8_t*)malloc(16);
    evp_bytes_to_key((uint8_t*)"stonejing", strlen("stonejing"), ikey, 16);
    crypto_derive_key("stonejing", mkey, 16);
    debug_hex(ikey, 16);
    debug_hex(mkey, 16);

    uint8_t *okm = (uint8_t*)malloc(16);
    hkdf_sha1(salt, 16, ikey, 16, (uint8_t*)"ss-subkey", strlen("ss-subkey"), okm, 16);
    debug_hex(okm, 16);

    uint8_t *mokm = (uint8_t*)malloc(16);
    crypto_hkdf(salt, 16, ikey, 16, (uint8_t*)"ss-subkey", strlen("ss-subkey"), mokm, 16);
    debug_hex(mokm, 16);

    printf("%d\n", strlen("stonejing"));
}


int main(void)
{
    test_crypto_aead_encrypt();

    return 0;
}