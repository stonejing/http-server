#include "encrypt.h"
#include "socks5shadowsocks.h"

int main()
{
    std::string server_address = "172.27.41.81";
    std::string method = "aes-256-cfb";
    std::string port = "2345";
    std::string password = "stonejing";

    run(server_address, port, method, password);
    return 0;
}