#include "Logging.h"
#include <assert.h>
#include "AsyncLogging.h"
#include <assert.h>
#include <time.h>
#include <sys/time.h>

/*
    log 需不需要创建一个单例的模式
*/
static pthread_once_t once_control_ = PTHREAD_ONCE_INIT;
static AsyncLogging* AsyncLogger_;

std::string Logger::logFileName_ = "webserver.log";

void once_init()
{
    AsyncLogger_ = new AsyncLogging(Logger::getLogFileName());
    // AsyncLogger_->start();
}

void output(const char* msg, int len)
{
    pthread_once(&once_control_, once_init);
    AsyncLogger_->Append(msg, len);
}

Logger::Impl::Impl(const char* fileName, int line)
    : stream_(), line_(line), basename_(fileName)
{
    formatTime();
}

void Logger::Impl::formatTime()
{
    struct timeval tv;
    time_t time_;
    char str_t[26] = {0};
    gettimeofday(&tv, NULL);
    time_ = tv.tv_sec;
    struct tm* p_time = localtime(&time_);
    strftime(str_t, 26, "%Y-%m-%d %H:%M:%S\n", p_time);
    stream_ << str_t;
}

Logger::Logger(const char* fileName, int line)
    : impl_(fileName, line)
{ }

Logger::~Logger()
{
    impl_.stream_ << " -- " << impl_.basename_ << ':' << impl_.line_ << '\n';
    const LogStream::Buffer& buf(stream().buffer());
    printf("write info to buffer.\n");
    output(buf.data(), buf.length());
}