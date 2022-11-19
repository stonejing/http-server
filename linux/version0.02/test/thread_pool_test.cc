#include "../event_loop_thread_pool.h"
#include <unistd.h>

int main()
{  
    ThreadPool t(5);
    std::shared_ptr<EventLoop> c = t.GetNextThread();
    usleep(1000);
    c->PushString("1");
    c->loop();
    c = t.GetNextThread();
    c->PushString("2");
    c->loop();

    return 0;
}