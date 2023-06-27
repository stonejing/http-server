#include "webserver.h"
#include <iostream>
#include <sys/epoll.h>

WebServer::WebServer(EventLoop* loop)
    : thread_pool_(new ThreadPool(7)),
      loop_(loop), listen_fd_(0)
{
    log_info("Start http server.");
    char server_path[200];
    getcwd(server_path, 200);
    std::string current_path = std::string(server_path);
    std::string root = "/root";
    root_path_ = current_path + root;
    std::cout << root_path_ << std::endl;
}

void WebServer::EventListen()
{
    // 网络编程基础步骤，设置 socket 的基础步骤，TCP 连接
    listen_fd_ = socket(PF_INET, SOCK_STREAM, 0);
    assert(listen_fd_ >= 0);
    int ret = 0;
    struct sockaddr_in address;
    bzero(&address, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_port = htons(2345);
    address.sin_addr.s_addr = INADDR_ANY;

    int reuse = 1;
    setsockopt(listen_fd_, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

    struct linger tmp = {1, 0};
    setsockopt(listen_fd_, SOL_SOCKET, SO_LINGER, &tmp, sizeof(tmp));

    ret = bind(listen_fd_, (struct sockaddr*)&address, sizeof(address));
    assert(ret != -1);
    ret = listen(listen_fd_, 5);
    assert(ret != -1);
    log_info("webserver start: %s: %d", "localhost: ", 2345);
}

void WebServer::Start()
{
    std::shared_ptr<Channel> accept_channel_(new Channel(loop_, listen_fd_));
    accept_channel_->set_events(EPOLLIN);
    accept_channel_->set_read_callback(std::bind(&WebServer::HandleNewConnection, this));
    loop_->AddtoPoller(accept_channel_);
    loop_->loop();
}

void WebServer::HandleNewConnection()
{
    struct sockaddr_in client_address;
    memset(&client_address, 0, sizeof(struct sockaddr_in));
    socklen_t client_addr_len = sizeof(client_address);   
    int accept_fd = 0;
    accept_fd = accept(listen_fd_, (struct sockaddr*)&client_address, &client_addr_len);
    auto loop = thread_pool_->GetNextThread();
    log_info("get new connection");
    setnonblocking(accept_fd);

    // 这里出现了生命周期的问题，如果没有下一行，这个智能指针就会直接销毁，就会报错
    std::shared_ptr<HttpConnection> request(new HttpConnection(loop.get(), accept_fd));
    // auto request = new HttpConnection(loop_, accept_fd);
    requests_[accept_fd] = request;
    loop->AddtoPoller(request->get_channel());
}