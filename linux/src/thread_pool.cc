#include "thread_pool.h"

std::shared_ptr<EventLoop> ThreadPool::GetNextThread()
{
    std::shared_ptr<EventLoop> loop = loops_[next_];
    next_ = (next_ + 1) % thread_numbers_;
    std::cout << next_ << "th loop" << std::endl;
    return loop;
}