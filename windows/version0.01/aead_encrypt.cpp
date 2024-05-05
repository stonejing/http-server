#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <openssl/evp.h>
#include <openssl/md5.h>
#include <openssl/hmac.h>
#include <openssl/rand.h>
#include <openssl/sha.h>
#include <openssl/kdf.h>

#include "dbg.hpp"
#include "aead_encrypt.hpp"


int rand_bytes(unsigned char *buf, int num)
{
	return RAND_bytes(buf, num);
}

// 用户 密码 生成 16位 key
void evp_bytes_to_key(const uint8_t *input, size_t input_len, uint8_t *key, size_t key_len)
{
    uint8_t round_res[16] = { 0 };
    size_t cur_pos = 0;

    uint8_t *buf = (uint8_t*)malloc(input_len + 16);
    memcpy(buf, input, input_len);

    while(cur_pos < key_len)
    {
        if(cur_pos == 0)
        {
            MD5(buf, input_len, round_res);
        }
        else
        {
            memcpy(buf, round_res, 16);
            memcpy(buf + 16, input, input_len);
            MD5(buf, input_len + 16, round_res);
        }
        for(int p = cur_pos; p < key_len && p < cur_pos + 16; p++) 
        {
            key[p] = round_res[p - cur_pos];
        }
        cur_pos += 16;
    }
    free(buf);
}

void increase_nonce(uint8_t *nonce, size_t bytes)
{
    uint16_t c = 1;
    for(size_t i = 0; i < bytes; ++i)
    {
        c += nonce[i];
        nonce[i] = c & 0xff;
        c >>= 8;
    }
}

// 生成加密用的 key
bool hkdf_sha1(const uint8_t *salt, size_t salt_len, const uint8_t *ikm,
	       size_t ikm_len, const uint8_t *info, size_t info_len,
	       uint8_t *okm, size_t okm_len)
{
	size_t outlen = okm_len;
	EVP_PKEY_CTX *pctx = EVP_PKEY_CTX_new_id(EVP_PKEY_HKDF, NULL);

	if (EVP_PKEY_derive_init(pctx) <= 0)
		return false;
	if (EVP_PKEY_CTX_hkdf_mode(pctx,
				   EVP_PKEY_HKDEF_MODE_EXTRACT_AND_EXPAND) <= 0)
		return false;
	if (EVP_PKEY_CTX_set_hkdf_md(pctx, EVP_sha1()) <= 0)
		return false;
	if (EVP_PKEY_CTX_set1_hkdf_salt(pctx, salt, salt_len) <= 0)
		return false;
	if (EVP_PKEY_CTX_set1_hkdf_key(pctx, ikm, ikm_len) <= 0)
		return false;
	if (EVP_PKEY_CTX_add1_hkdf_info(pctx, info, info_len) <= 0)
		return false;
	if (EVP_PKEY_derive(pctx, okm, &outlen) <= 0)
		return false;

	EVP_PKEY_CTX_free(pctx);
	return true;
}


void encrypt_reset_iv(cipher_ctx_t *ctx)
{
    EVP_CIPHER_CTX_reset(ctx->evp);
    EVP_EncryptInit_ex(ctx->evp, EVP_aes_128_gcm(), NULL, ctx->skey, ctx->nonce);
    increase_nonce(ctx->nonce, 12);
}

void decrypt_reset_iv(cipher_ctx_t *ctx)
{
    EVP_CIPHER_CTX_reset(ctx->evp);
    EVP_DecryptInit_ex(ctx->evp, EVP_aes_128_gcm(), NULL, ctx->skey, ctx->nonce);
    increase_nonce(ctx->nonce, 12);
}

bool aead_text_encrypt(cipher_ctx_t *ctx, const uint8_t *plaintext, int plaintext_len,
                uint8_t *ciphertext, int *ciphertext_len)
{
    int len;
    
    if(1 != EVP_EncryptUpdate(ctx->evp, ciphertext, &len, plaintext, plaintext_len))
        return false;

    *ciphertext_len = len;

    if(1 != EVP_EncryptFinal_ex(ctx->evp, ciphertext + len, &len))
        return false;
   
    uint8_t *tag = (uint8_t*)malloc(16);
    *ciphertext_len += len;
    
    if(1 != EVP_CIPHER_CTX_ctrl(ctx->evp, EVP_CTRL_AEAD_GET_TAG, 16, tag))
        return false;

    memcpy(ciphertext + *ciphertext_len, tag, 16);
    // printf("tag: ");
    // debug_hex((const uint8_t*)tag, 16);
    *ciphertext_len += 16;
    free(tag);
    return true;
}


bool aead_text_decrypt(cipher_ctx_t *ctx, const uint8_t *ciphertext, 
                size_t ciphertext_len, uint8_t *tag, 
                uint8_t *plaintext, size_t *plaintext_len)
{
    int len = 0;

    // decrypt successive memory, can not decryp same memory multiple times
    if(!EVP_DecryptUpdate(ctx->evp, plaintext, &len, ciphertext, ciphertext_len))
    {
        log_err("text decrypt update failed.");
        return false;
    }
    *plaintext_len = len;

    if(!EVP_CIPHER_CTX_ctrl(ctx->evp, EVP_CTRL_GCM_SET_TAG, 16, tag))
    {
        log_err("text decrypt ctrl failed.");
        return false;
    }

    if(!EVP_DecryptFinal_ex(ctx->evp, plaintext + len, &len))
    {
        log_err("text decrypt final failed.");
        return false;
    }
    *plaintext_len += len;

    return true;
}

void aead_encrypt(cipher_ctx_t *ctx, uint8_t *plaintext, int plaintext_len, uint8_t *ciphertext, int *ciphertext_len)
{
    if(!ctx->init)
    {
        // evp bytes to key
        uint8_t *ikey = (uint8_t*)malloc(16);
        evp_bytes_to_key((const uint8_t*)"stonejing", strlen("stonejing"), ikey, 16);
        uint8_t encrypt_salt[16] = {0x63, 0xb4, 0x60, 0xd3, 0xce, 0x25, 0x0f, 0x60, 0xc0, 0x3f, 0xa9, 0xca, 0x33, 0x93, 0xcb, 0xd5};
        // generate random salt
        ctx->salt = (uint8_t*)malloc(16);
        rand_bytes(ctx->salt, 16);
        for(int i = 0; i < 16; i++)
        {
            ctx->salt[i] = encrypt_salt[i];
        }
        // generate nonce from 0
        ctx->nonce = (uint8_t*)calloc(1, 12);
        // generate subkey from (key, salt, info)
        ctx->skey = (uint8_t*)malloc(16);
        hkdf_sha1(ctx->salt, 16, ikey, 16, (const uint8_t*)"ss-subkey", strlen("ss-subkey"), ctx->skey, 16);
        // printf("ikey: ");
        // debug_hex(ikey, 16);
        // printf("salt: ");
        // debug_hex(ctx->salt, 16);
        // printf("nonce: ");
        // debug_hex(ctx->nonce, 12);
        // printf("skey: ");
        // debug_hex(ctx->skey, 16);

        // initialize encryption with subkey and nonce
        encrypt_reset_iv(ctx);
        memcpy(ciphertext, ctx->salt, 16);

        // log_info("salt copied.");

        uint8_t prefix[2] = { 0 };
        prefix[0] = plaintext_len >> 8;
        prefix[1] = plaintext_len;
        // printf("prefix: ");
        // debug_hex(prefix, 2);
        int chunk_len = 0;

        int temp1;
        aead_text_encrypt(ctx, prefix, 2, ciphertext + 16, &temp1);
        
        // log_info("aead length encrypted.");
        // increase_nonce(ctx->nonce, 12);

        encrypt_reset_iv(ctx);
        int cipher_len;
        aead_text_encrypt(ctx, plaintext, plaintext_len, ciphertext + 16 + 2 + 16, &cipher_len);

        // increase_nonce(ctx->nonce, 12);
        // log_info("aead payload encrypt.");
        chunk_len = temp1 + cipher_len + 16;

        *ciphertext_len = chunk_len;

        // log_info("ciphertext_len: %d\n", *ciphertext_len);

        ctx->init = 1;
        return;
    }
    else
    {
        uint8_t prefix[2] = { 0 };
        prefix[0] = plaintext_len >> 8;
        prefix[1] = plaintext_len;
        int chunk_len = 0;

        int temp1;
        encrypt_reset_iv(ctx);
        aead_text_encrypt(ctx, prefix, 2, ciphertext, &temp1);
        
        encrypt_reset_iv(ctx);
        int cipher_len;
        aead_text_encrypt(ctx, plaintext, plaintext_len, ciphertext + 2 + 16, &cipher_len);

        chunk_len = temp1 + cipher_len;

        *ciphertext_len = chunk_len;
    }
    return;
}

void aead_decrypt(cipher_ctx_t *ctx, uint8_t *ciphertext, int ciphertext_len, uint8_t *plaintext, int* plaintext_len)
{
    memcpy(ctx->chunk->data + ctx->chunk->len, ciphertext, ciphertext_len);
    ctx->chunk->len += ciphertext_len;    

    if(!ctx->init)
    {
        int salt_len = 16;
        // evp bytes to key
        uint8_t *ikey = (uint8_t*)malloc(16);
        evp_bytes_to_key((const uint8_t*)"stonejing", strlen("stonejing"), ikey, 16);

        ctx->salt = (uint8_t*) malloc(16);
        memcpy(ctx->salt, ciphertext, 16);

        ctx->nonce = (uint8_t*)calloc(1, 12);
        // generate subkey from (key, salt, info)
        ctx->skey = (uint8_t*)malloc(16);
        hkdf_sha1(ctx->salt, 16, ikey, 16, (const uint8_t*)"ss-subkey", strlen("ss-subkey"), ctx->skey, 16);
        ctx->init = 1;

        memcpy(ctx->chunk->data, ctx->chunk->data + salt_len, ctx->chunk->len - salt_len);
        ctx->chunk->len -= salt_len;
    }

    size_t plen = 0;
    size_t cidx = 0;

    if(ctx->chunk->len > 34)
    {
        if(ctx->chunk->idx == 0)
        {
            uint8_t buf_len[2];
            size_t decode_len;
            decrypt_reset_iv(ctx);
            if(!aead_text_decrypt(ctx, ctx->chunk->data, 2, ctx->chunk->data + 2, buf_len, &decode_len))
            {
                log_err("decode payload length failed.");
                *plaintext_len = -1;
                return;
            }
            ctx->chunk->idx = (uint16_t)buf_len[0] << 8 | buf_len[1];
            // log_info("decode_len: %d, payload_length: %d", decode_len, ctx->chunk->idx);
        }
  

        if(ctx->chunk->idx > ctx->chunk->len - 2 - 16 - 16)
        {
            *plaintext_len = 0;
            return;
        }

        size_t pp_len = 0;
  
        decrypt_reset_iv(ctx);
        if(!aead_text_decrypt(ctx, ctx->chunk->data + 2 + 16, ctx->chunk->idx, ctx->chunk->data + 2 + 16 + ctx->chunk->idx, plaintext + plen, &pp_len))
        {
            *plaintext_len = -1;
            log_err("decrypt payload error.");
            return;
        }

        // log_info("decrypt payload length: %d %d", ctx->chunk->idx, pp_len);

        plen += pp_len;
        size_t chunk_len = ctx->chunk->idx + 2 + 16 + 16;

        ctx->chunk->len -= chunk_len;

        memcpy(ctx->chunk->data, ctx->chunk->data + chunk_len, ctx->chunk->len);

        ctx->chunk->idx = 0;

    }
    *plaintext_len = plen;

    return;
}