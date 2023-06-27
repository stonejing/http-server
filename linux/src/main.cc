#include "webserver.h"
#include <iostream>

int main(void)
{
    EventLoop mainloop;
    WebServer webserver(&mainloop);
    webserver.EventListen();
    webserver.Start();
    return 0;
}