#pragma once
#include <assert.h>
#include <string.h>
#include <string>

const int kSmallBuffer = 4000;
const int kLargeBuffer = 4000 * 1000;

template<int SIZE>
class FixedBuffer
{
public:
    FixedBuffer()
    : cur_(data_)
    { }

    ~FixedBuffer() { }
    
    void Append(const char* buf, size_t len)
    {
        if(avail() > static_cast<int>(len))
        {
            memcpy(cur_, buf, len);
            cur_ += len;
        }
    }

    const char* data() const { return data_; }
    int length() const { return static_cast<int>(cur_ - data_); }

    char* current() { return cur_; }
    int avail() const { return static_cast<int>(end() - cur_); }
    void add(size_t len) { cur_ += len; }
    
    void reset() { cur_ = data_; }
    void bzero() { memset(data_, 0, sizeof(data_)); }

private:
    const char* end() const { return data_ + sizeof(data_); }
    char data_[SIZE];
    char* cur_;
};

class LogStream
{
public:
    // typedef FixedBuffer<kSmallBuffer> Buffer;
    using Buffer = FixedBuffer<kSmallBuffer>;

    LogStream& operator<<(bool v)
    {
        buffer_.Append(v ? "1" : "0", 1);
        return *this;
    }

    LogStream& operator<<(short);
    LogStream& operator<<(unsigned short);
    LogStream& operator<<(int);
    LogStream& operator<<(unsigned int);
    LogStream& operator<<(long);
    LogStream& operator<<(unsigned long);
    LogStream& operator<<(long long);
    LogStream& operator<<(unsigned long long);

    LogStream& operator<<(const void*);

    LogStream& operator<<(float y)
    {
        *this << static_cast<double>(y);
        return *this;
    }
    LogStream& operator<<(double);
    LogStream& operator<<(long double);

    LogStream& operator<<(char v)
    {
        buffer_.Append(&v, 1);
        return *this;
    }

    LogStream& operator<<(const char* str)
    {
        if(str)
            buffer_.Append(str, strlen(str));
        else
            buffer_.Append("(null)", 0);
        return *this;
    }

    LogStream& operator<<(const unsigned char* str)
    {
        return operator<<(reinterpret_cast<const char*>(str));
    }

    LogStream& operator<<(const std::string& v)
    {
        buffer_.Append(v.c_str(), v.size());
        return *this;
    }

    void Append(const char* data, int len) { buffer_.Append(data, len); }
    const Buffer& buffer() const { return buffer_; }
    void resetBuffer() { buffer_.reset(); }

private:
    template<typename T>
    void formatInteger(T);

    Buffer buffer_;
    static const int kMaxNumberSize = 32;

};