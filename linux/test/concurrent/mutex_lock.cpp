#include <iostream>
#include <pthread.h>
#include <unistd.h>

using namespace std;

char buffer;
int buffer_has_item = 0;
pthread_mutex_t mutex;

void writer()
{
    while(1)
    {
        pthread_mutex_lock(&mutex);
        buffer_has_item++;
        cout << "write " << buffer_has_item << endl;
        pthread_mutex_unlock(&mutex);
        usleep(1000 * 100);
    }
}

void *reader(void *ptr)
{
    while(1)
    {
        pthread_mutex_lock(&mutex);
        if(buffer_has_item > 0)
        {
            cout << "read " << buffer_has_item << endl;
            buffer_has_item--;
        }
        pthread_mutex_unlock(&mutex);
        usleep(1000 * 200);
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