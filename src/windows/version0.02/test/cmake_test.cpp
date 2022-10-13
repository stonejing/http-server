#include <iostream>
#include <WinSock2.h>
#include <Windows.h>
#include <thread>
#include <vector>
#include <unordered_map>
#include <string>
#include <openssl/evp.h>
#include <openssl/opensslv.h>
#include <openssl/crypto.h>

using namespace std;

void thread_test()
{
    SOCKET dumb = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    std::cout << "generate dummy socket: " << dumb << std::endl;

    FD_SET read_set;
       
    std::cout << "openssl encrypt start." << std::endl;
    unsigned char outbuf_encrypt[256];
    int outlen_encrpyt, tmplen_encypt;
    unsigned char outbuf_cipher[256];
    int outlen_cipher, tmplen_cipher;
    char* outfile = "result.txt";

    unsigned char key[] = { 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15 };
    unsigned char iv[] = { 1,2,3,4,5,6,7,8 };

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
        return;
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
        return;
    }

    char str[] = "stonejing\n";

    fwrite(str, 1, sizeof(str), out);
    fwrite(outbuf_encrypt, 1, outlen_encrpyt, out);
    fclose(out);

    std::cout << "openssl end. write to file. " << std::endl;

    while (true)
    {
        FD_ZERO(&read_set);
        FD_SET(dumb, &read_set);
        std::cout << "select in sub thread" << std::endl;
        int ret = select(0, &read_set, NULL, NULL, NULL);
        if (ret == SOCKET_ERROR) return;
        std::cout << "test." << std::endl;
    }
}

int main(void)
{
    WSADATA wsaData;

    if (WSAStartup(0x0202, &wsaData) != 0)
    {
        WSACleanup();
        std::cout << "none" << std::endl;
        return -1;
    }

    std::vector<int> test;

    std::cout << test.size() << std::endl;

    std::unordered_map<int, int> test_2;
    for (auto& a : test_2)
    {
        std::cout << "test_2" << std::endl;
    }


    int num = 3;

    std::thread t(thread_test);
    t.detach();

    while (num != 0)
    {
        std::cout << "test" << std::endl;
        num--;
    }
    std::cout << "main thread" << std::endl;
    SOCKET ttt = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    FD_SET main_set;
    FD_ZERO(&main_set);
    while (true)
    {
        FD_ZERO(&main_set);
        FD_SET(ttt, &main_set);
        std::cout << "select in main thread" << std::endl;
        int ret = select(0, &main_set, NULL, NULL, NULL);
        std::cout << "one loop" << std::endl;
    }

    return 1;
}