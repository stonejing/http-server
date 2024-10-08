#include "webserver.h"
#include "channel.h"

Webserver::Webserver(int thread_num, int port) : 
    listen_fd(socket_bind_listen_tcp_v4(port)),
    threadpool(std::make_unique<ThreadPool>(thread_num)),
    port(port)
{
    handle_for_sigpipe();

    if(threadpool->threadPoolStatus()) quit = false;
}

void Webserver::handleNewConnection(int accept_fd)
{
    std::shared_ptr<EventLoop> loop = threadpool->getNextLoop();
    loop->setNewSocket(accept_fd);
}

void Webserver::serverAcceptStart()
{
    struct sockaddr_in client_address;
    socklen_t client_addr_len = sizeof(client_address);

    fd_set read_fd;
    FD_ZERO(&read_fd);

    while(!quit)
    {
        FD_ZERO(&read_fd);
        FD_SET(listen_fd, &read_fd);
        int ret = select(listen_fd + 1, &read_fd, NULL, NULL, NULL);
        if(ret < 0)
        {
            LOG_ERR("select error: %d %d", ret, errno);
            abort();
        }
        if(FD_ISSET(listen_fd, &read_fd))
        {
            int accept_fd = accept(listen_fd, (struct sockaddr*)&client_address, &client_addr_len);
            if(accept_fd <= 0)
            {
                LOG_WARN("accept failure ", errno, accept_fd);
                continue;
            }
            setnonblocking(accept_fd);
            handleNewConnection(accept_fd);
        }
    }
}

