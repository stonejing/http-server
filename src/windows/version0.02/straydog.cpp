#include "straydog.h"

#include <string>

std::string address = "127.0.0.1";
std::string amazon = "52.192.95.12";
std::string password = "stonejing";

void StrayDog::StartServer()
{
    ProxyServer proxy_server(
        amazon,
        5000,
        password,
        0,
        5005
    );

    proxy_server.EventListen();
    proxy_server.ServerStart();
}