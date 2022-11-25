#include "straydog.h"
#include "proxyserver.h"

int main(void)
{
    // StrayDog straydog;
    // straydog.StartServer();

    std::string address = "127.0.0.1";
    // std::string amazon = "52.192.95.12";
    int remote_port = 5000;
    std::string password = "stonejing";

    ProxyServer proxy_server(
        address,
        remote_port,
        password,
        0,
        11380
    );

    int ret = proxy_server.EventListen();
    if(ret == -1) return 1;

    proxy_server.ServerStart();
    
    return 1;
}