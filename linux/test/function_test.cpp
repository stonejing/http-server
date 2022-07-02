#include <iostream>
#include <vector>
#include <openssl/evp.h>
#include <string.h>
#include <functional>

using namespace std;

class Foo
{
public:
    Foo();
    void foo1()
    {
        cout << "a" << endl;
        // cout << a << endl;
    }
    void foo2()
    {
        std::function<void(void)> temp = std::bind(&Foo::foo1, this);
        cout << "b" << endl;
    }
};

int func_test(int arr[], int size)
{
    for(int i = 0; i < size; i++)
    {
        printf("%dth: %d ", i, arr[i]);
    }
    printf("\nTEST.\n");
    return -1;
}

int main()
{
    int c = 2;
    std::vector<int> test = {1, 2, 3, 4, 5};

    test[c++] = 1;
    int arr[5] = {1, 2, 3, 4, 5};
    func_test(arr, 5);

    for(auto i : test)
    {
        cout << i << " ";
    }
    cout << endl;

    uint8_t enc_key[32];
    uint8_t iv[16];

    const EVP_CIPHER* cipher = EVP_get_cipherbyname("aes-256-cfb");
    int enc_key_len = EVP_BytesToKey(cipher, EVP_md5(), NULL, (uint8_t *)"stonejing", 
            strlen("stonejing"), 1, enc_key, iv);
    int env_iv_len = EVP_CIPHER_iv_length(cipher);

    for(int i = 0; i < 32; i++)
    {
        
    }

    return 0;
}