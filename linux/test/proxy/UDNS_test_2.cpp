#include <arpa/inet.h>
extern "C" {
#include <udns.h>  // Include the UDNS C header
}

#include <iostream>
#include <vector>
#include <string>
#include <thread>
#include <chrono>
#include <netdb.h>
#include <sys/select.h>
#include <sys/socket.h>

using namespace std;

// Callback function for query completion
void queryCallback(struct dns_ctx *ctx, struct dns_rr_a4 *result, void *data)
{
    cout << "DNS: " << result->dnsa4_nrr << endl;
}

int main() {
    std::vector<std::string> hosts = {"www.google.com", "www.github.com"};

    // Initialize the UDNS library
    dns_init(NULL, 0);

    struct dns_ctx *ctx = dns_new(NULL);
    ::dns_set_opt(ctx, DNS_OPT_TIMEOUT, 2);

    int sockfd = dns_open(ctx);
    cout << sockfd << endl;
    // Start asynchronous queries for each host
    fd_set read_set;
    fd_set write_set;

    FD_ZERO(&read_set);
    FD_ZERO(&write_set);
    FD_SET(sockfd, &read_set);
    // FD_SET(sockfd, &write_set);

    struct dns_query* query = dns_submit_a4(ctx, "www.google.com", 0, queryCallback, NULL);
    if(!query)
    {
        cout << "dns_submit_a4 error" << endl;
        return -1;
    }
    time_t now = time(NULL);
    int t = dns_timeouts(NULL, -1, now);

    cout << "query: " << t << " " << endl;

    while(true)
    {
        int ret = select(sockfd + 1, &read_set, &write_set, NULL, NULL);
        if(ret < 0)
        {
            cout << "select error" << endl;
            break;
        }
        if(FD_ISSET(sockfd, &write_set))
        {
            dns_ioevent(ctx, 0);
            cout << "IO EVENT." << endl;
        }
        if(FD_ISSET(sockfd, &read_set))
        {
            dns_ioevent(ctx, 0);
            cout << "IO EVENT." << endl;
        }
    }

    // Wait for the queries to complete (you might need to implement a proper event loop)
    // For demonstration purposes, we'll just sleep for a short time.
    std::this_thread::sleep_for(std::chrono::seconds(5));

    // Cleanup
    dns_free(ctx);

    return 0;
}
