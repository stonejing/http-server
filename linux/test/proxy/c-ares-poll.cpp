#include <sys/time.h>
#include <sys/poll.h>
#include <arpa/inet.h>
#include <arpa/nameser.h>
#include <ares.h>
#include <pthread.h>
#include <deque>
#include <netdb.h>
#include <string>
#include <cstring>
#include <cstdio>
#include <iostream>

using namespace std;

class dns_resolver_t
{
public:
    dns_resolver_t() : channel_(NULL)
    {
        // int ret = ares_init(&channel_);
        // if (ret != ARES_SUCCESS)
        // {
        //     err_info_ = ares_strerror(ret);
        // }
        struct ares_options options;
        int optmask = ARES_OPT_FLAGS;
        options.flags = ARES_FLAG_NOCHECKRESP;
        options.flags |= ARES_FLAG_STAYOPEN;
        options.flags |= ARES_FLAG_IGNTC; // UDP only
        
        int status = ares_init_options(&channel_, &options, optmask);
        ::ares_set_socket_callback(channel_, &dns_resolver_t::ares_socket_create_callback, this);
        cout << "construct over." << endl;
    }

    ~dns_resolver_t()
    {
        if (channel_)
        {
            ares_destroy(channel_);
        }
    }

    int resolve(int af, const std::string& domain, int& timeout/*ms*/, void* addr, size_t addr_len)
    {
        dns_res_t res = {NULL, addr, addr_len};
        ares_gethostbyname(channel_, domain.c_str(), af, dns_callback, &res);
        struct timeval last, now;
        gettimeofday(&last, NULL);
        int nfds = 1;
        while (nfds)
        {
            struct timeval *tvp, tv, store = {timeout / 1000, (timeout % 1000) * 1000};

            // return maximum time to wait
            tvp = ares_timeout(channel_, &store, &tv);
            int timeout_ms = tvp->tv_sec * 1000 + tvp->tv_usec / 1000;

            printf("timeout_ms(%d)\n", timeout_ms);
            nfds = dns_wait_resolve(channel_, timeout_ms);
            printf("dns_wait_resolve nfds(%d)\n", nfds);

            gettimeofday(&now, NULL);
            timeout -= (now.tv_sec - last.tv_sec) * 1000 + (now.tv_usec - last.tv_usec) / 1000;
            last = now;
        }

        if (res.error_info)
        {
            err_info_ = res.error_info;
            printf("resolve err(%s)\n", res.error_info);
            return -1;
        }
        return 0;
    }

    operator bool() const
    {
        return channel_;
    }

    const std::string& error_info() const
    {
        return err_info_;
    }

private:
    dns_resolver_t(const dns_resolver_t&);
    dns_resolver_t& operator=(const dns_resolver_t&);

    int dns_fd = -1;

    struct dns_res_t
    {
        const char* error_info;
        void* address;
        size_t len;
    };

    static int ares_socket_create_callback(int sock, int type, void *data)
    {
        dns_resolver_t* dr = (dns_resolver_t*)data;
        dr->dns_fd = sock;
        cout << "socket create callback: " << sock << endl;
        return 0;
    }

    static void dns_callback(void* arg, int status, int timeouts, struct hostent* hptr)
    {
        // TODO: get the first address
        printf("dns_callback status(%d) timeouts(%d)\n", status, timeouts);

        dns_res_t& res = *(dns_res_t*)arg;
        if (status != ARES_SUCCESS)
        {
            res.error_info = ares_strerror(status);
            printf("dns_callback err(%s)\n", res.error_info);
            return;
        }

        if (AF_INET == hptr->h_addrtype)
        {
            char** pptr = hptr->h_addr_list;
            if (*pptr)
            {
                memcpy(res.address, *pptr, res.len);
                return;
            }

            res.error_info = "no invalid address get";
            printf("dns_callback err(%s)\n", res.error_info);
        }
        else
        {
            res.error_info = "addrtype not supported";
            printf("addrtype(%d) not supported\n", hptr->h_addrtype);
        }
    }

    int dns_wait_resolve(ares_channel channel_, int timeout_ms)
    {
        if (timeout_ms < 0)
        {
            ares_process_fd(channel_, ARES_SOCKET_BAD, ARES_SOCKET_BAD);

            // TODO: 这里若不执行 ares_cancel，channel_ 析构会出现 coredump
            ares_cancel(channel_);
            return 0;
        }
        int nfds;
        int bitmask;
        ares_socket_t socks[ARES_GETSOCK_MAXNUM];
        struct pollfd pfd[ARES_GETSOCK_MAXNUM];
        int i;
        int num = 1;

        pfd[0].events |= POLLIN | POLLRDNORM;
        pfd[0].fd = dns_fd;

        // bitmask = ares_getsock(channel_, socks, ARES_GETSOCK_MAXNUM);

        // for (i = 0; i < ARES_GETSOCK_MAXNUM; i++)
        // {
        //     pfd[i].events = 0;
        //     pfd[i].revents = 0;
        //     if (ARES_GETSOCK_READABLE(bitmask, i))
        //     {
        //         pfd[i].fd = socks[i];
        //         pfd[i].events |= POLLRDNORM | POLLIN;
        //     }
        //     if (ARES_GETSOCK_WRITABLE(bitmask, i))
        //     {
        //         pfd[i].fd = socks[i];
        //         pfd[i].events |= POLLWRNORM | POLLOUT;
        //     }
        //     if (pfd[i].events != 0)
        //     {
        //         num++;
        //     }
        //     else
        //     {
        //         break;
        //     }
        // }

        if (num)
        {
            nfds = poll(pfd, num, timeout_ms/*milliseconds */);
        }
        else
        {
            nfds = 0;
        }

        if (!nfds)
        {
            ares_process_fd(channel_, ARES_SOCKET_BAD, ARES_SOCKET_BAD);

            // TODO: 这里若不执行 ares_cancel，在超时错误时，channel_ 析构会出现 coredump
            ares_cancel(channel_);
        }
        else
        {
            for (i = 0; i < num; i++)
            {
                ares_process_fd(channel_,
                                (pfd[i].revents & (POLLRDNORM | POLLIN)) ? pfd[i].fd : ARES_SOCKET_BAD,
                                (pfd[i].revents & (POLLWRNORM | POLLOUT)) ? pfd[i].fd : ARES_SOCKET_BAD);
            }
        }
        return nfds;
    }

    ares_channel channel_;
    std::string err_info_;
};

int main()
{
    struct sockaddr_in sa = {};
    std::string domain = "www.stonejing.link";
    int timeout_ms = 1000;
    dns_resolver_t dr;
    if (dr)
    {
        int ret = dr.resolve(AF_INET, domain, timeout_ms, &sa.sin_addr.s_addr, sizeof(sa.sin_addr.s_addr));
        if (0 != ret)
        {
            printf("dr.resolve ret(%d) err(%s)\n", ret, dr.error_info().c_str());
            return 0;
        }
        char strIP[INET_ADDRSTRLEN] = {0};
        inet_ntop(AF_INET, &(sa.sin_addr), strIP, INET_ADDRSTRLEN);
        printf("%s\n", strIP);
    }
    else
    {
        printf("dns_resolver_t init err(%s)\n", dr.error_info().c_str());
        return 0;
    }
    return 0;
}