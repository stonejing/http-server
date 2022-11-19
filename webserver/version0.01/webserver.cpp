#include "webserver.h"
#include <ctime>
// #include "signal_handler.h"

static int m_pipefd[2];

void sig_handler(int sig)
{
    int save_errno = errno;
    int msg = sig;
    send(m_pipefd[1], (char*)&msg, 1, 0);
    errno = save_errno;
}

void add_sig(int sig)
{
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = sig_handler;
    sa.sa_flags |= SA_RESTART;
    sigfillset(&sa.sa_mask);
    assert(sigaction(sig, &sa, NULL) != -1);
}

WebServer::WebServer()
{
    log_info("Start http server.");
    client_data = new ClientData[MAX_FD];

    for(int i = 0; i < MAX_FD; i++)
    {
        client_data[i].http_connection = new http_conn();
    }

    epoll = EpollEvent{};
    m_listenfd = -1;
    
    char server_path[200];
    getcwd(server_path, 200);
    char root[6] = "/root";
    m_root = (char*)malloc(strlen(server_path) + strlen(root) + 1);
    strcpy(m_root, server_path);
    strcpy(m_root, root);
    socketpair(PF_UNIX, SOCK_STREAM, 0, m_pipefd);

}

void WebServer::event_listen()
{
    // 网络编程基础步骤，设置 socket 的基础步骤，TCP 连接
    m_listenfd = socket(PF_INET, SOCK_STREAM, 0);
    assert(m_listenfd >= 0);

    int ret = 0;
    struct sockaddr_in address;
    bzero(&address, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_port = htons(2345);
    address.sin_addr.s_addr = INADDR_ANY;

    int reuse = 1;
    setsockopt(m_listenfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

    struct linger tmp = {1, 0};
    setsockopt(m_listenfd, SOL_SOCKET, SO_LINGER, &tmp, sizeof(tmp));

    ret = bind(m_listenfd, (struct sockaddr*)&address, sizeof(address));
    assert(ret != -1);
    ret = listen(m_listenfd, 5);
    assert(ret != -1);

    epoll.AddFd(m_listenfd, false);
    setnonblocking(m_pipefd[1]);
    // epoll.AddFd(m_pipefd[0], false);

    signal(SIGPIPE, SIG_IGN);
    // add_sig(SIGHUP);
    // add_sig(SIGCHLD);
    // add_sig(SIGTERM);
    // add_sig(SIGALRM);
    // add_sig(SIGINT);
    
    // alarm(TIMESLOT);
}

void WebServer::event_loop()
{
    bool timeout = false;
    bool stop_server = false;
    int epoll_fd = epoll.GetEpollfd();
    while(!stop_server)
    {
        int number = epoll_wait(epoll_fd, events, MAX_EVENT_NUMBER, -1);
        log_info("epoll event number: %d", number);
        if(number < 0 && errno != EINTR)
        {
            log_err("epoll failure.");
            break;  
        }
        if(number < 0)
        {
            perror("epoll wait error");
        }
        for(int i = 0; i < number; i++)
        {
            int sockfd = events[i].data.fd;
            if(sockfd == m_listenfd)
            {
                bool flag = accept_client();
                if(flag == false) continue;
            }
            else if(events[i].events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR))
            {
                log_warn("timer event.");
            }
            // else if((sockfd == m_pipefd[0]) && (events[i].events & EPOLLIN))
            // {
            //     log_warn("signal event.");
            //     bool flag = deal_signal(timeout, stop_server);
            // }
            else if(events[i].events & EPOLLIN)
            {
                log_info("read event.");
                deal_read(sockfd);
            }
            else if(events[i].events & EPOLLOUT)
            {
                log_info("write event.");
                deal_write(sockfd);
            }
            else
            {
                log_err("unknown error.");
            }
        }
        tw.takeAllTimeout();
    }
    log_info("bye!!!");
}

bool WebServer::accept_client()
{
    struct sockaddr_in client_address;
    socklen_t client_addrlength = sizeof(client_address);
    int conn_sock;
    while((conn_sock = accept(m_listenfd, (struct sockaddr*)&client_address, (socklen_t*)&client_addrlength)) > 0)
    {
        /* 这两句是调试用 */
        int reuse = 1;
        setsockopt(conn_sock, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
        log_info("accept new client.");
        epoll.AddFd(conn_sock, true);
        client_data[conn_sock].http_connection->set_sockfd(conn_sock);
        void* temp = client_data[conn_sock].http_connection;
        std::function<void(void*)> tmp = std::bind(&WebServer::CallbackFunction, this, client_data[conn_sock].http_connection);
        TimeWheelTimer* timer = tw.AddTimer(3000, tmp, client_data[conn_sock].http_connection);
        client_data[conn_sock].timer = timer;
    }
    return true;
}

bool WebServer::deal_signal(bool& timeout, bool& stop_server)
{
    int sig;
    char signals[1024];
    int ret = recv(m_pipefd[0], signals, sizeof(signals), 0);
    if(ret <= 0) return false;
    else
    {
        for(int i = 0; i < ret; ++i)
        {
            switch(signals[i])
            {
                case SIGALRM:
                {
                    timeout = true;
                    break;
                }
                case SIGTERM:
                {
                    log_info("stop server.");
                    stop_server = true;
                    break;
                }
            }
        }
    }
    return true;
}

void WebServer::deal_read(int sockfd)
{
    client_data[sockfd].http_connection->read();
    client_data[sockfd].http_connection->process();
    epoll.ModifyFd(sockfd, EPOLLOUT);
}

void WebServer::deal_write(int sockfd)
{
    if(client_data[sockfd].http_connection->write())
    {
        epoll.ModifyFd(sockfd, EPOLLOUT);
    }
    else
    {
        epoll.ModifyFd(sockfd, EPOLLIN);
    }
}
