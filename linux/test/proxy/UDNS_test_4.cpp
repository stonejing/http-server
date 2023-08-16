#include <iostream>
#include <vector>
#include <sys/epoll.h>
#include <udns.h> // Replace with the actual UDNS header
#include <unistd.h>

const int MAX_EVENTS = 10;

struct DnsQuery {
    int fd;
    bool resolved = false;
    // Add necessary fields based on the UDNS API in version 0.4
};

int main() {
    // Initialize UDNS resolver
    struct dns_ctx *ctx = dns_new(NULL);

    // List of hostnames to query
    std::vector<std::string> hostnames = {"google.com", "example.com", "openai.com"};

    // Create epoll instance
    int epollfd = epoll_create1(0);
    if (epollfd == -1) {
        perror("Failed to create epoll instance");
        return 1;
    }

    // Create and add UDNS queries to epoll
    std::vector<DnsQuery> dnsQueries;

    for (const std::string &hostname : hostnames) {
        DnsQuery dnsQuery;
        // Initialize dnsQuery fields based on the UDNS API in version 0.4

        struct epoll_event dnsEvent;
        dnsEvent.events = EPOLLIN; // Listen for read events
        dnsEvent.data.ptr = &dnsQuery;

        if (epoll_ctl(epollfd, EPOLL_CTL_ADD, dnsQuery.fd, &dnsEvent) == -1) {
            perror("Failed to add DNS query socket to epoll");
            dns_free(ctx);
            close(epollfd);
            return 1;
        }

        dnsQueries.push_back(dnsQuery);
    }

    // Main event loop
    struct epoll_event events[MAX_EVENTS];

    while (!dnsQueries.empty()) {
        int ready = epoll_wait(epollfd, events, MAX_EVENTS, -1);
        if (ready == -1) {
            perror("epoll_wait error");
            break;
        }

        for (int i = 0; i < ready; ++i) {
            DnsQuery *dnsQuery = static_cast<DnsQuery*>(events[i].data.ptr);
            
            // Process DNS response based on the UDNS API in version 0.4
            // Set dnsQuery.resolved = true when the query is resolved

            if (dnsQuery->resolved) {
                std::cout << "Resolved IP address for " << dnsQuery->hostname << ": " << dnsQuery->resolved_ip << std::endl;
                dnsQueries.erase(dnsQuery);
            }
        }
    }

    // Cleanup
    for (const DnsQuery &dnsQuery : dnsQueries) {
        // Cleanup resources based on the UDNS API in version 0.4
    }

    dns_free(ctx);
    close(epollfd);

    return 0;
}
