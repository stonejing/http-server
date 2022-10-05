#include "straydog.h"

#include <string>

std::string address = "127.0.0.1";
std::string password = "stonejing";

void StrayDog::StartServer()
{
    ProxyServer proxy_server(
        address,
        5000,
        password,
        0,
        5005
    );

    proxy_server.EventListen();
    proxy_server.ServerStart();
}