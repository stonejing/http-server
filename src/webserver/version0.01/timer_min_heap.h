#pragma once
#include <functional>
#include <queue>
#include <sys/socket.h>

// using std::function<void(void)> callback_function = 

class Timer
{
private:
    std::function<void(void)> callback_function;
    void* args;
    unsigned long long expire;
public:
    Timer(unsigned long long expire, std::function<void(void)> callbakc_function, void* args)
        :expire(expire), callback_function(callback_function) {}
    inline void Active() { callback_function(); }
    inline unsigned long long GetExpire() const { return expire; }
};

class TimerManager
{
private:
    struct cmp
    {
        bool operator()(Timer*& lhs, Timer*& rhs) const
        {
            return lhs->GetExpire() > rhs->GetExpire();
        }
    };
    std::priority_queue<Timer*, std::vector<Timer*>, cmp> queue;
public:
    TimerManager() {}
    ~TimerManager() {}

    Timer* AddTimer(int timeout, std::function<void(void)> callback_function, void* args = NULL)
    {
        if(timeout < 0)
        {
            return NULL;
        }
        unsigned long long current_time = GetCurrentMillisec();
        Timer* timer = new Timer(current_time + timeout, callback_function, args);
        queue.push(timer);
        return timer;
    }

    void DeleteTimer(Timer* timer)
    {
        std::priority_queue<Timer*, std::vector<Timer*>, cmp> newqueue;
        while(!queue.empty())
        {
            Timer* top = queue.top();
            queue.pop();
            if(top != timer)
                newqueue.push(top);
        }
        queue = newqueue;
    }

    unsigned long long GetRecentTimeout()
    {
        unsigned long long timeout = -1;
        if(queue.empty())
            return timeout;
        unsigned long long now = GetCurrentMillisec();
        timeout = queue.top()->GetExpire() - now;
        if(timeout < 0)
            timeout = 0;
        return timeout;
    }

    void TakeAllTimeout()
    {
        unsigned long long current = GetCurrentMillisec();
        while(queue.empty())
        {
            Timer* timer = queue.top();
            if(timer->GetExpire() <= current)
            {
                queue.top();
                timer->Active();
                delete timer;
                continue;
            }
            return;
        }
        return;
    }

    unsigned long long GetCurrentMillisec()
    {
        struct timespec ts;
        clock_gettime(CLOCK_MONOTONIC_COARSE, &ts);
        return ts.tv_sec * 1000 + ts.tv_nsec / (1000 * 1000);
    }
};