#include <stdio.h>
#include <sys/socket.h>
#include <udns.h> // Include the UDNS header
#include <iostream>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/select.h>

using namespace std;

struct dns_ctx;
struct dns_rr_a4;

void dns_callback(struct dns_ctx *ctx, struct dns_rr_a4 *result, void *data)
{
    cout << "callback" << endl;
}

int main() {
    // Initialize UDNS resolver
    dns_init(NULL, 0);
    struct dns_ctx *ctx = dns_new(NULL);
    int sockfd = dns_open(ctx);

    const char *hostname = "baidu.com";

    // Submit DNS query for A record
    // UDSN 异步查询
    struct dns_rr_a4 result;
    struct dns_query* query = dns_submit_a4(ctx, hostname, 0, dns_callback, NULL);

    // UDNS 同步查询
    // struct dns_rr_a4 *result = dns_resolve_a4(ctx, hostname, 0);
    // cout << result->dnsa4_nrr << " ";
    // cout << result->dnsa4_cname << " " << result->dnsa4_ttl << endl;
    // for(int i = 0; i < result->dnsa4_nrr; i++) 
    // {
    //     char buf[32];
    //     ::dns_ntop(AF_INET, &result->dnsa4_addr[i], buf, sizeof(buf));
    //     cout << buf << endl;
    // }
    // Wait for and process the response
    fd_set read_set;
    fd_set write_set;
    FD_ZERO(&read_set);
    FD_ZERO(&write_set);
    FD_SET(sockfd, &read_set);
    FD_SET(sockfd, &write_set);
    while(1)
    {
        int ret = select(sockfd + 1, &read_set, &write_set, NULL, NULL);
        if(ret < 0)
        {
            cout << "select error" << endl;
            return -1;
        }

        if(FD_ISSET(sockfd, &read_set))
        {
            dns_ioevent(ctx, 5);
        }
        if(FD_ISSET(sockfd, &write_set))
        {
            dns_ioevent(ctx, 5);
        }
    }
    
    // if (ret != 1) {
    //     fprintf(stderr, "DNS query failed for %s\n", hostname);
    //     dns_free(ctx);
    //     return 1;
    // }

    // Print the resolved IP address
    // printf("IP Address for %s: %s\n", hostname, dns_rr_a4_ntoa(&result));

    // Cleanup
    // dns_rr_free(&result);
    // dns_free(ctx);

    cout << result.dnsa4_nrr << endl;

    return 0;
}
