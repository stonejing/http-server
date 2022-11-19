#include "webserver.h"

int main()
{
    WebServer server{};
    server.event_listen();
    server.event_loop();

    return 0;
}