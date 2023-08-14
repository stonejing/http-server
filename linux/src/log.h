#pragma once

#include <cstdio>
#include <ctime>
#include <fstream>
#include <iostream>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <cstdarg>
#include <string>
#include <sys/time.h>
#include <cstring>
#include <thread>
#include <chrono>
#include <sstream>
#include <iomanip>

using namespace std;

#define CONSOLE
class CLogger;

#define LOG_INFO(msg, ...) Log.log_info(__FILE__, __LINE__, msg, ##__VA_ARGS__)
#define LOG_WARN(msg, ...) Log.log_warn(__FILE__, __LINE__, msg, ##__VA_ARGS__)
#define LOG_ERR(msg, ...) Log.log_err(__FILE__, __LINE__, msg, ##__VA_ARGS__)

// synchronized LOG, the same function for input and output

class CLogger
{
private:
    CLogger()  
    {

    }
    ~CLogger()
    {
        if(log_file.is_open())
        {
            log_file.flush();
            log_file.close();
        }
    }
    CLogger(const CLogger&);
	CLogger& operator=(const CLogger&) = delete;

private:
    std::string get_time_stamp_c() 
    {
        struct timeval tv;
        gettimeofday(&tv, nullptr);
        time_t currTime = tv.tv_sec;
        struct tm* timeinfo = localtime(&currTime);
        char buffer[20];
        std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", timeinfo);
        return std::string(buffer);
    }

    std::string get_time_stamp_cpp() 
    {
        auto now = std::chrono::system_clock::now();
        auto time = std::chrono::system_clock::to_time_t(now);
        char buffer[20];
        std::strftime(buffer, 20, "%Y-%m-%d %H:%M:%S", std::localtime(&time));
        return std::string(buffer);
    }

private:
    std::queue<string> buffer;
    
    ofstream log_file;

    std::mutex mut;
    std::condition_variable cond;

public:
    static CLogger& getInstance()
    {
        static CLogger instance;
        return instance;
    }

    void init(const string& file_name)
    {
        log_file.open(file_name, ios::ate);
    }

    template<typename... Args>
    void log_info(const char* file_name, int file_line, const char* format, Args... args)
    {
        std::unique_lock<std::mutex> lock(mut);

        std::ostringstream oss;
        oss << "\033[32;1m[INFO]\033[0m\033[34m(" << file_name << ":" << file_line << ")\033[0m ";
        oss << get_time_stamp_c();
        // oss << " " << std::hash<std::thread::id>{}(std::this_thread::get_id()) << " ";
        oss << " " << format;
        // Use the stream to format the arguments
        if constexpr (sizeof...(args) > 0) {
            (oss << ... << args);
        }

        std::cout << oss.str() << std::endl;

        // char buffer[1024];
        // #ifdef CONSOLE
        //     int offset = std::snprintf(buffer, sizeof(buffer), "\033[32;1m[INFO]\033[0m\033[34m(%s:%d)\033[0m ", file_name, file_line);
        //     offset += get_time_stamp_c(buffer + offset);
        //     offset += std::snprintf(buffer + offset, sizeof(buffer) - offset, " %zu ", std::hash<std::thread::id>{}(std::this_thread::get_id()));
        //     if (offset >= 0 && offset < sizeof(buffer)) 
        //     {
        //         std::snprintf(buffer + offset, sizeof(buffer) - offset, format.c_str(), args...);
        //     }
        //     std::cout << buffer << std::endl;
        // #else
        //     int offset = std::snprintf(buffer, sizeof(buffer), "[INFO] (%s:%d) ", file_name, file_line);
        //     offset += get_time_stamp_cpp(buffer + offset);
        //     offset += std::snprintf(buffer + offset, sizeof(buffer) - offset, " %zu ", std::hash<std::thread::id>{}(std::this_thread::get_id()));
        //     if (offset >= 0 && offset < sizeof(buffer)) 
        //     {
        //         std::snprintf(buffer + offset, sizeof(buffer) - offset, format, args...);
        //     }
        //     std::unique_lock<std::mutex> lock(mut);
        //     log_file << buffer << "\n";
        // #endif
        lock.unlock();
    }

    template<typename... Args>
    void log_warn(const char* file_name, int file_line, const std::string& format, Args... args)
    {
        std::unique_lock<std::mutex> lock(mut);

        std::ostringstream oss;
        oss << "\033[33;1m[WARN]\033[0m\033[34m(" << file_name << ":" << file_line << ")\033[0m ";
        oss << get_time_stamp_c();
        oss << " " << std::hash<std::thread::id>{}(std::this_thread::get_id()) << " ";
        oss << format;
        // Use the stream to format the arguments
        if constexpr (sizeof...(args) > 0) {
            (oss << ... << args);
        }

        std::cout << oss.str() << std::endl;
        lock.unlock();
    }

    template<typename... Args>
    void log_err(const char* file_name, int file_line, const std::string& format, Args... args)
    {
        std::unique_lock<std::mutex> lock(mut);

        std::ostringstream oss;
        oss << "\033[31;1m[ERR]\033[0m\033[34m(" << file_name << ":" << file_line << ")\033[0m ";
        oss << get_time_stamp_c();
        // oss << " " << std::hash<std::thread::id>{}(std::this_thread::get_id()) << " ";
        oss << " " << format;
        // Use the stream to format the arguments
        if constexpr (sizeof...(args) > 0) {
            (oss << ... << args);
        }

        std::cout << oss.str() << std::endl;
        lock.unlock();
    }
};