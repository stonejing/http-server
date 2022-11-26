#ifndef LOG_H_
#define LOG_H_

#include <iostream>
#include <atomic>
#include <string>

class Log
{
public:
    ~Log()
    {
        std::cout << "destructor called." << std::endl;
    }

    Log(const Log&) = delete;
    Log& operator=(const Log&) = delete;

    static Log& get_instance()
    {
        static Log instance;
        return instance;
    }

    template<typename T> 
    Log& operator<<(T s)
    {
        std::cout << s;
        return *this; 
    }

private:
    Log()
    {
    }
};

#define LOG_WARN (Log::get_instance() << "\u001b[33m[WARN]\u001b[0m " << __FILE__ << ":" << __LINE__ << " ")
#define LOG_ERROR (Log::get_instance() << "\u001b[31m[ERROR]\u001b[0m " << __FILE__ << ":" << __LINE__ << " ")
#define LOG_INFO (Log::get_instance() << "\u001b[32m[INFO]\u001b[0m " << __FILE__ << ":" << __LINE__ << " ")

#endif