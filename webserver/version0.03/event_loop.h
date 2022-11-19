#include <functional>
#include <vector>

class EventLoop
{
public:
    using Functor = std::function<void()>;
    EventLoop();
    ~EventLoop();

    /* while loop, created by thread */
    void loop();
    /* never called */
    void quit();
    /*  */
    void RunInLoop(Functor&& cb);
    
    void QueueInLoop(Functor&& cb);

    int QueueSize() const;
};