#include <sys/select.h>
#include <ares.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <time.h>
#include <iostream>

void dns_callback(void* arg, int status, int timeouts, struct hostent* host) 
{
    if (status == ARES_SUCCESS)
        std::cout << host->h_name << std::endl;
    else
        std::cout << "lookup failed: " << ares_strerror(status) << std::endl;
}

void main_loop(ares_channel& channel) 
{
    int nfds, count;
    fd_set readers, writers;
    timeval tv, *tvp;
    while (1) 
    {
        FD_ZERO(&readers);
        FD_ZERO(&writers);
        // 获取c-ares内部需要处理的fd
        nfds = ares_fds(channel, &readers, &writers);
        if (nfds == 0) break;
        // 获取超时时间
        tvp = ares_timeout(channel, NULL, &tv);
        // 调用select来等待fd达到条件
        count = select(nfds, &readers, &writers, NULL, tvp);
        // 将fd的新状态交于c-ares内部处理
        ares_process(channel, &readers, &writers);
    }
}

int main(int argc, char** argv) 
{
    struct in_addr ip;
    int res;
    if (argc < 2) {
        std::cout << "usage: " << argv[0] << " ip.address\n";
        return 1;
    }
    // 读取输入的ip地址
    inet_aton(argv[1], &ip);
    ares_library_init(ARES_LIB_INIT_ALL);
    // 初始化channel
    ares_channel channel;
    if ((res = ares_init(&channel)) != ARES_SUCCESS) {
        std::cout << "ares feiled: " << res << '\n';
        return 1;
    }
    // 调用查询api，当main_loop中驱动时间运转时，查询有结果时会触发回调dns_callback
    ares_gethostbyaddr(channel, &ip, sizeof ip, AF_INET, dns_callback, NULL);
    main_loop(channel);
    ares_library_cleanup();
    return 0;
}