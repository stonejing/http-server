#include <queue>
#include <thread>
#include <condition_variable>
#include <iostream>
#include <mutex>
#include <queue>

using namespace std;
template<typename T>
class threadsafe_queue
{
public:
    threadsafe_queue() {}
    ~threadsafe_queue() {}

    template<typename... Args>
    void push(string level, string file_name, int file_line, string error_message, Args&&... args)
    {
        std::lock_guard<std::mutex> lock(mut);
        string buff = "[" + level + "]" + " " + file_name + ":" + to_string(file_line) + " " + error_message;
        cout << buff << " " << endl;
        data_queue.push(std::move(buff));
        data_cond.notify_one();
    }
      
    void wait_and_pop(T& value)
    {
        std::unique_lock<std::mutex> lock(mut);
        data_cond.wait(lock, [this]{ return !data_queue.empty(); });
        value = std::move(data_queue.front());
        data_queue.pop();
    }

    std::shared_ptr<T> wait_and_pop()
    {
        std::unique_lock<std::mutex> lock(mut);
        data_cond.wait(lock, [this]{ return !data_queue.empty(); });
        std::shared_ptr<T> res(std::make_shared<T>(std::move(data_queue.front())));
        data_queue.pop();
        return res;
    }

    bool try_pop(T& value)
    {
        std::lock_guard<std::mutex> lock(mut);
        if(data_queue.empty())
            return false;
        value = std::move(data_queue.front());
        data_queue.pop();
        return true;
    }

    std::shared_ptr<T> try_pop()
    {
        std::lock_guard<std::mutex> lock(mut);
        if(data_queue.empty())
            return std::shared_ptr<T>();
        std::shared_ptr<T> res(std::make_shared<T>(std::move(data_queue.front())));
        data_queue.pop();
        return res;
    }

    bool empty() const
    {
        std::lock_guard<std::mutex> lock(mut);
        return data_queue.empty();
    }

private:
    queue<string> data_queue;
    mutex mut;
    condition_variable data_cond;
};

threadsafe_queue<int> q;

#define log_info(M, ...) q.push("INFO", __FILE__, __LINE__, M, ##__VA_ARGS__);


int main(void)
{
    log_info("error in:", 2);
    return 0;
}