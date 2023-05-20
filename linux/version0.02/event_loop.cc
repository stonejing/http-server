#include "event_loop.h"

EventLoop::EventLoop() : 
        poller_(new Epoller()),
        thread_id_(std::this_thread::get_id())
{ 
    std::cout << "create new event loop" << std::endl;
    std::cout << "thread id: " << thread_id_ << std::endl;
}

void EventLoop::loop()
{
    while(true)
    {
        auto result = poller_->poll();
        for(auto& item : result)
        {
            item->HandleEvent();
        }
    }
}

void EventLoop::AddtoPoller(std::shared_ptr<Channel> channel)
{
    poller_->EpollAdd(channel);
}

void EventLoop::UpdatePoller(std::shared_ptr<Channel> channel)
{
    poller_->EpollModify(channel);
}

void EventLoop::RemovePoller(std::shared_ptr<Channel> channel)
{
    poller_->EpollDelete(channel);
}