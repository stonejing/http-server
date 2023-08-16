#include <sys/socket.h>
#include <time.h>
#include <iostream>
#include <netdb.h>
#include <arpa/inet.h>
#include <ares.h>
#include <cstdio>
#include <unistd.h>

void dns_callback (void* arg, int status, int timeouts, struct hostent* host)
{
    if (status == ARES_SUCCESS) 
    {
        puts(host->h_name);
        puts(inet_ntoa(*(struct in_addr*)host->h_addr));
    }
    else
        std::cout << "lookup failed: " << status << '\n';
}

void main_loop(ares_channel &channel)
{
    int nfds, count;
    fd_set readers, writers;
    timeval tv, *tvp;

    ares_gethostbyname(channel, "http.stonejing.link", AF_INET, dns_callback, NULL);
    ares_gethostbyname(channel, "github.com", AF_INET, dns_callback, NULL);

    while (1) 
    {
        FD_ZERO(&readers);
        FD_ZERO(&writers);
        nfds = ares_fds(channel, &readers, &writers);
        FD_SET(STDIN_FILENO, &readers);
        // if (nfds == 0) 
        // {
        //     std::cout << "nfds == 0\n";
        //     break;
        // }
        // std::cout << nfds << std::endl;
        // tvp = ares_timeout(channel, NULL, &tv);
        count = select(nfds + 1, &readers, &writers, NULL, tvp);
        if (count < 0) 
        {
            std::cout << "select failed: " << count << '\n';
            return;
        }
        if(FD_ISSET(0, &readers))
        {
            char buf[1024];
            int n = read(0, buf, sizeof(buf));
            buf[n] = '\0';
            // ares_process(channel, &readers, 0);
            ares_gethostbyname(channel, buf, AF_INET, dns_callback, NULL);
            continue;
        }
        else {
            ares_process(channel, &readers, 0);
        }
    }

}

int main(int argc, char **argv)
{
    int res;
    if(argc < 2 ) 
    {
        std::cout << "usage: " << argv[0] << " domain_name\n";
        return 1;
    }

    ares_channel channel;
    if((res = ares_init(&channel)) != ARES_SUCCESS) 
    {
        std::cout << "ares feiled: " << res << '\n';
        return 1;
    }
    ares_gethostbyname(channel, argv[1], AF_INET, dns_callback, NULL);
    main_loop(channel);
    return 0;
}