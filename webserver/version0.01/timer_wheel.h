#pragma once

#include <functional>
#include <vector>
#include <algorithm>
#include <iostream>
#include "http_conn.h"

class TimeWheelTimer
{
public:
    TimeWheelTimer(int rotations, int slot, std::function<void(void*)> callback_function, void* args)
        : rotations_(rotations), slot_(slot), callback_function_(callback_function), args_(args) {}
    inline int get_rotations() { return rotations_; }
    inline int DecreaseRotations() { return --rotations_; }
    /* 更新定时器时间 */
    inline int set_ratations() { return rotations_++; }
    inline void Active() { callback_function_(args_); }
    inline int get_slot() { return slot_; }

private:
    int rotations_;
    int slot_;
    std::function<void(void*)> callback_function_;
    void* args_;        
};

class TimeWheel
{
public:
    TimeWheel(int total_slots)
        : total_slots_(total_slots), current_slot_(0),
          slots_(total_slots_, std::vector<TimeWheelTimer *>()), start_time_(GetCurrentMilliseconds())
    {
    }

    ~TimeWheel()
    {
        for(std::vector<TimeWheelTimer*> vect : slots_)
        {
            for(TimeWheelTimer* timer : vect)
                delete timer;
        }
    }

    unsigned long long GetCurrentMilliseconds()
    {
        struct timespec ts;
        clock_gettime(CLOCK_MONOTONIC_COARSE, &ts);
        return ts.tv_sec * 1000 + ts.tv_nsec / (1000 * 1000);
    }

    TimeWheelTimer* AddTimer(unsigned long long timeout, 
                        std::function<void(void*)> func, void * args)
    {
        int slot = 0;
        if(timeout < 0) return NULL;
        slot = (current_slot_ + (timeout % total_slots_)) % total_slots_;
        auto timer = new TimeWheelTimer(timeout / total_slots_, slot, func, args);
        slots_[slot].push_back(timer);
        printf("Add timer in slot: %d\n", slot);
        return timer;
    }

    void DeleteTimer(TimeWheelTimer* timer)
    {
        if(!timer) return;
        std::vector<TimeWheelTimer*>::iterator it = std::find(slots_[timer->get_slot()].begin(), slots_[timer->get_slot()].end(), timer);
        if(it != slots_[timer->get_slot()].end())
        {
            slots_[timer->get_slot()].erase(it);
        }
    }

    void tick()
    {
        for (auto it = slots_[current_slot_].begin(); it != slots_[current_slot_].end();)
        {
            if ((*it)->get_rotations() > 0)
            {
                (*it)->DecreaseRotations();
                ++it;
                continue;
            }
            else
            {
                TimeWheelTimer *timer = *it;
                timer->Active();
                it = slots_[current_slot_].erase(it);
                delete timer;
            }
        }

        current_slot_ = ++current_slot_ % total_slots_;
    }

    void takeAllTimeout()
    {
        int now = GetCurrentMilliseconds();
        printf("Take all timeout.\n");
        int cnt = now - start_time_;
        for (int i = 0; i < cnt; ++i)
            tick();
        start_time_ = now;
    }

private:
    int total_slots_;
    int current_slot_;
    unsigned long long start_time_;
    std::vector<std::vector<TimeWheelTimer*>> slots_;
};