#include <iostream>
#include <thread>
#include <mutex>
#include <queue>

class Singleton
{
private:
	Singleton() { };
	~Singleton() { };
	Singleton(const Singleton&);
	Singleton& operator=(const Singleton&) = default;
    int a = 0;

    std::mutex m;
public:
    void print()
    {
        std::unique_lock lock(m);
        std::cout << "TEST: " << a << std::endl;
        a = a + 1;
        lock.unlock();
    }
	static Singleton& getInstance() 
    {
		static Singleton instance;
		return instance;
	}
};

class bar 
{
public:
    void foo(int a) 
    {
        std::cout << "hello from member function" << std::endl;
    }
};

void print()
{
    Singleton& c = Singleton::getInstance();
    c.print();
}

void print_c(char c)
{
    std::cout << "c" << std::endl;
}

int main(void)
{
    std::thread t[10];

    for(int i = 0; i < 10; i++)
    {
        t[i] = std::thread(print);
    }

    for (int i = 0; i < 10; i++) 
    {
        t[i].join();
    }
}