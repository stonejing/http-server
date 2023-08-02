#include <iostream>
#include <pthread.h>
#include <unistd.h>

using namespace std;

int buffer_has_item = 0;
pthread_mutex_t mutex;
pthread_cond_t cond;

void writer()
{
    while(1)
    {
        pthread_mutex_lock(&mutex);
        buffer_has_item += 3;
        cout << "write: " << buffer_has_item << endl;
        pthread_mutex_unlock(&mutex);
        // 激活阻塞的 reader 线程
        pthread_cond_signal(&cond);
        usleep(1000 * 200);
    }
}

void *reader(void *ptr)
{
    while(1)
    {
        pthread_mutex_lock(&mutex);
        if(buffer_has_item <= 0)
            // 暂时解锁，把资源让出，等待 writer 线程信号
            // 被激活后自动加锁
            pthread_cond_wait(&cond, &mutex);
        
        cout << "read: " << buffer_has_item << endl;
        buffer_has_item--;
        pthread_mutex_unlock(&mutex);
    }
}

int main(void)
{
    pthread_t id;
    pthread_mutex_init(&mutex, NULL);
    pthread_create(&id, NULL, reader, NULL);
    writer();
    return 0;
}