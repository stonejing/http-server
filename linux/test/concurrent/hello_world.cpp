#include <iostream>
#include <pthread.h>
#include <unistd.h>

using namespace std;

void* run(void* ptr)
{
    for(int i = 0; i < 3; i++)
    {
        sleep(1);
        cout << "hello world: " << i << endl;
    }
    return 0;
}

int main(void)
{
    pthread_t id;
    int ret = 0;
    ret = pthread_create(&id, NULL, run, NULL);
    if(ret)
    {
        cout << "create thread failed." << endl;
        return 0;
    }

    pthread_join(id, NULL);
    return 0;
}
