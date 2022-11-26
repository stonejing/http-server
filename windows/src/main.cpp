#include "server.h"

int main(void)
{
    // StrayDog straydog;
    // straydog.StartServer();

    std::string address = "127.0.0.1";
    // std::string amazon = "52.192.95.12";
    int remote_port = 5000;
    std::string password = "stonejing";

    Server server(
        address,
        remote_port,
        password,
        0,
        11380
    );

    int ret = server.EventListen();
    if(ret == -1) return 1;

    server.ServerStart();
    
    return 1;
}