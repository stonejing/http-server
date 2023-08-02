#include <iostream>
#include <pthread.h>
#include <unistd.h>

using namespace std;

pthread_key_t key;

void* run(void* ptr)
{
    int value = *(int*) ptr;
    int temp = 2;

    pthread_setspecific(key, &temp);
    void* v = 0;
    v =  pthread_getspecific(key);
    cout << v << endl;
    for(int i = 0; i < 3; i++)
    {
        sleep(1);
        cout << "hello world: " << i << " "<< endl;
    }
    return 0;
}

int main(void)
{
    pthread_t id;
    int ret = 0;
    int value = 0;
    ret = pthread_create(&id, NULL, run, &value);
    if(ret)
    {
        cout << "create thread failed." << endl;
        return 0;
    }

    pthread_join(id, NULL);
    return 0;
}
