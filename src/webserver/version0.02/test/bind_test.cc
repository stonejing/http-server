#include <functional>
#include <memory>
#include <iostream>
#include <vector>

class Channel
{
public:
    Channel(){}
    ~Channel(){}
    void set_read(std::function<void()>&& cb)
    {
        callback_ = cb;
    }
public:
    std::function<void()> callback_;
};

class Foo
{
public:
    Foo(int test) : test_(test), channel_(new Channel())
    {
        channel_->set_read(std::bind(&Foo::print, this));
    }

    void print()
    {
        std::cout << "test: " << test_ << std::endl;
    }

    std::shared_ptr<Channel> get_channel()
    {
        return channel_;
    }
private:
    int test_;
    std::shared_ptr<Channel> channel_;
};

int main()
{
    std::shared_ptr<Foo> foo(new Foo(5));
    auto t = foo->get_channel();
    t->callback_();
    return 0;
}