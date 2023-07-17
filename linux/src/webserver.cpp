#include "webserver.h"
#include "utils.h"
#include <errno.h>

Webserver::Webserver(int thread_num, int port) : 
    listen_fd(socket_bind_listen_tcp_v4(port)),
    threadpool(std::make_unique<ThreadPool>(thread_num)),
    port(port)
{
    handle_for_sigpipe();

    if(threadpool->threadPoolStatus()) quit = false;
    LOG_INFO("webserver constructed");
}

void Webserver::serverAcceptStart()
{
    struct sockaddr_in client_address;
    socklen_t client_addr_len = sizeof(client_address);
    int accept_fd = 0;

    fd_set read_fd;
    FD_ZERO(&read_fd);

    LOG_INFO("select listen the accept socket");
    int accept_socket = 0;
    while(!quit)
    {
        FD_SET(listen_fd, &read_fd);
        int ret = select(listen_fd + 1, &read_fd, NULL, NULL, NULL);
        if(ret < 0)
        {
            LOG_ERR("select error: %d %d", ret, errno);
            abort();
        }
        LOG_INFO("select one time %d", accept_socket++);
        if(FD_ISSET(listen_fd, &read_fd))
        {
            int accept_fd = accept(listen_fd, (struct sockaddr*)&client_address, &client_addr_len);
            if(accept_fd <= 0)
            {
                LOG_WARN("accept failure");
                continue;
            }
            setnonblocking(accept_fd);
            handleNewConnection(accept_fd);
        }
    }
}

