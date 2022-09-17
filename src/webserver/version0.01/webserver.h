#pragma

#include "http_conn.h"
// #include "timer_lst.h"
#include "timer_wheel.h"
#include "dbg.h"
#include "epoll_event.h"
// #include "timer_min_heap.h"
#include "utils.h"
#include <memory>

const int MAX_FD = 65536;
const int MAX_EVENT_NUMBER = 10000;
const int TIMESLOT = 2;
const int TCP_BUFFER_SIZE = 512;
const int UDP_BUFFER_SZIE = 1024;

struct ClientData
{
    http_conn* http_connection;
    TimeWheelTimer* timer;
};

class WebServer
{

public:
    char* m_root;

    ClientData* client_data;

    int m_listenfd;

    epoll_event events[MAX_EVENT_NUMBER];

    EpollEvent epoll;
    // SignalHandler signal_handler;
    TimeWheel tw = TimeWheel(60000);

public:
    WebServer();
    ~WebServer(){};

    void event_listen();
    void event_loop();

    bool accept_client();

    bool deal_signal(bool& time_out, bool& stop_server);
    void deal_read(int sockfd);
    void deal_write(int sockfd);

    void CallbackFunction(void* http_connection)
    {
        http_conn* temp = (http_conn*)http_connection;
        epoll_ctl(epoll.GetEpollfd(), EPOLL_CTL_DEL, temp->get_sockfd(), 0);
        assert(http_connection);
        close(temp->get_sockfd());
        printf("Callback, close socket.\n");
    }
};