#include "crypto.h"

Crypto::Crypto() : 
    enc_ctx_(EVP_CIPHER_CTX_new()),
    dec_ctx_(EVP_CIPHER_CTX_new()),
    encrypt_init_(false),
    decrypt_init_(false),
    payload_value_(0),
    decrypt_buffer_len_(0)
{

}

int Crypto::rand_bytes(uint8_t* buf, int num)
{
    return RAND_bytes(buf, num);
}

void Crypto::evp_bytes_to_key(const uint8_t* input, size_t input_len, uint8_t* key, size_t key_len)
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

// 生成加密用的 key
bool Crypto::hkdf_sha1(const uint8_t *salt, size_t salt_len, const uint8_t *ikm,
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

void Crypto::enc_increase_nonce()
{
    uint16_t c = 1;
    for(size_t i = 0; i < 12; ++i)
    {
        c += enc_nonce[i];
        enc_nonce[i] = c & 0xff;
        c >>= 8;
    }
}

void Crypto::dec_increase_nonce()
{
    uint16_t c = 1;
    for(size_t i = 0; i < 12; ++i)
    {
        c += dec_nonce[i];
        dec_nonce[i] = c & 0xff;
        c >>= 8;
    }
}

void Crypto::encrypt_reset_iv()
{
    EVP_CIPHER_CTX_reset(enc_ctx_);
    EVP_EncryptInit_ex(enc_ctx_, EVP_aes_128_gcm(), NULL, enc_subkey, enc_nonce);
    enc_increase_nonce();
}

void Crypto::decrypt_reset_iv()
{
    EVP_CIPHER_CTX_reset(dec_ctx_);
    EVP_DecryptInit_ex(dec_ctx_, EVP_aes_128_gcm(), NULL, dec_subkey, dec_nonce);
    dec_increase_nonce();
}

bool Crypto::aead_chunk_encrypt(const uint8_t *plaintext, int plaintext_len,
                                uint8_t *tag,
                                uint8_t *ciphertext, int &ciphertext_len)
{
    int len;
    if(EVP_EncryptUpdate(enc_ctx_, ciphertext, &len, plaintext, plaintext_len) != 1)
        return false;
    ciphertext_len = len;

    if(EVP_EncryptFinal_ex(enc_ctx_, ciphertext + len, &len) != 1)
        return false;
    ciphertext_len += len;
    if(EVP_CIPHER_CTX_ctrl(enc_ctx_, EVP_CTRL_AEAD_GET_TAG, 16, tag) != 1)
        return false;
    ciphertext_len += 16;
    return true;
}

int Crypto::aead_encrypt(char* plaintext, int plaintext_len,
                         char* ciphertext, int& ciphertext_len)
{
    int salt_len = 0;
    if(!encrypt_init_)
    {
        uint8_t ikey[16];
        evp_bytes_to_key((const uint8_t*)"stonejing", strlen("stonejing"), ikey, 16);
        uint8_t salt[16];
        rand_bytes(salt, 16);
        memset(enc_nonce, 0, 12);
        hkdf_sha1(salt, 16, ikey, 16, (const uint8_t*)"ss-subkey", strlen("ss-subkey"),
                  enc_subkey, 16);
        encrypt_reset_iv();
        memcpy(ciphertext, salt, 16);
        salt_len = 16;
        encrypt_init_ = true;
    }

    uint8_t prefix[2] = { 0 };
    prefix[0] = plaintext_len >> 8;
    prefix[1] = plaintext_len;

    int payload_length_len;
    aead_chunk_encrypt(prefix, 2, (uint8_t*)ciphertext + salt_len + 2, 
                        (uint8_t*)ciphertext + salt_len, payload_length_len);

    encrypt_reset_iv();
    int payload_len;
    aead_chunk_encrypt((uint8_t*)plaintext, plaintext_len, (uint8_t*)ciphertext + salt_len + 2 + 16 + plaintext_len,
                       (uint8_t*)ciphertext + salt_len + 2 + 16, payload_len);
    ciphertext_len = salt_len + 2 + 16 + plaintext_len + 16;
    encrypt_reset_iv();
    return 1;
}

bool Crypto::aead_chunk_decrypt(const uint8_t *ciphertext, int ciphertext_len,
                                uint8_t *tag,
                                uint8_t *plaintext, int &plaintext_len)
{
    int len = 0;
    if(EVP_DecryptUpdate(dec_ctx_, plaintext, &len, ciphertext, ciphertext_len) != 1)
    {
        return false;
    }
    plaintext_len = len;
    if(EVP_CIPHER_CTX_ctrl(dec_ctx_, EVP_CTRL_GCM_SET_TAG, 16, tag) != 1)
    {
        return false;
    }
    if(EVP_DecryptFinal_ex(dec_ctx_, plaintext + len, &len) != 1)
    {
        return false;
    }
    plaintext_len += len;
    return true;
}

int Crypto::aead_decrypt(char* ciphertext, int ciphertext_len, char* plaintext, int& plaintext_len)
{
    memcpy(decrypt_buffer_ + decrypt_buffer_len_, ciphertext, ciphertext_len);

    decrypt_buffer_len_ += ciphertext_len;

    if(!decrypt_init_)
    {
        uint8_t ikey[16];
        evp_bytes_to_key((const uint8_t*)"stonejing", strlen("stonejing"), ikey, 16);
        uint8_t salt[16];
        memcpy(salt, ciphertext, 16);
        memset(dec_nonce, 0, 12);
        hkdf_sha1(salt, 16, ikey, 16, (const uint8_t*)"ss-subkey", strlen("ss-subkey"), dec_subkey, 16);

        decrypt_init_ = true;

        decrypt_buffer_len_ -= 16;

        memcpy(decrypt_buffer_, decrypt_buffer_ + 16, decrypt_buffer_len_);
    }

    int plen = 0;

    while(decrypt_buffer_len_ > 34)
    {
        if(payload_value_ == 0)
        {
            uint8_t buf_len[2];
            int decode_len;
            decrypt_reset_iv();
            aead_chunk_decrypt((const uint8_t*)decrypt_buffer_, 2, (uint8_t*)(decrypt_buffer_ + 2),
                               buf_len, decode_len);
            payload_value_ = (uint16_t)buf_len[0] << 8 | buf_len[1];

            if(payload_value_ > 16383 || payload_value_ <= 0 || decode_len != 2)
            {
                log_err("payload length decryption error detected %d, decode_len: ", payload_value_, decode_len);
                exit(0);
            }

        }
        if(payload_value_ > decrypt_buffer_len_ - 2 - 16 - 16)
        {
            // need more data
            return 1;
        }

        int chunk_plen = 0;

        decrypt_reset_iv();
        aead_chunk_decrypt((const uint8_t*)(decrypt_buffer_ + 18), payload_value_, 
                            (uint8_t*)(decrypt_buffer_ + 18 + payload_value_), 
                            (uint8_t*)(plaintext + plen), chunk_plen);
        if(chunk_plen != payload_value_)
        {
            log_err("decrypt payload error: %d %d", payload_value_, chunk_plen);
            exit(0);
        }

        plen += chunk_plen;
        plaintext_len = plen;

        int chunk_len = payload_value_ + 2 + 16 + 16;
        decrypt_buffer_len_ -= chunk_len;
        memcpy(decrypt_buffer_, decrypt_buffer_ + chunk_len, decrypt_buffer_len_);
        payload_value_ = 0;
    }

    plaintext_len = plen;

    return 1;
}