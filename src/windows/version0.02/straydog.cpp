#include "straydog.h"

#include <string>

std::string address = "127.0.0.1";
// std::string amazon = "52.192.95.12";
std::string password = "stonejing";

void StrayDog::StartServer()
{
    ProxyServer proxy_server(
        address,
        6005,
        password,
        0,
        6000
    );

    int ret = proxy_server.EventListen();
    if(ret == -1) exit(0);
    proxy_server.ServerStart();
}