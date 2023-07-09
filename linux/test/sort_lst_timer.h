#ifndef LST_TIMER_H_
#define LST_TIMER_H_

#include <time.h>
#include <netinet/in.h>

#define BUFFER_SIZE 64
class util_timer;

struct client_data
{
    sockaddr_in address;
    int sockfd;
    char buf[BUFFER_SIZE];
    util_timer* timer;
};

class util_timer
{
public:
    util_timer(): prev(NULL), next(NULL) {}

public:
    time_t expire;
    void (*cb_func)(client_data*);
    client_data* user_data;
    util_timer* prev;
    util_timer* next;
};

class sort_timer_lst
{

};

#endif