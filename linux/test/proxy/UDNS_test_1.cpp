#include <stdio.h>
#include <sys/socket.h>
#include <udns.h> // Include the UDNS header
#include <iostream>
#include <unistd.h>
#include <arpa/inet.h>

using namespace std;

struct dns_ctx;
struct dns_rr_a4;

int main() {
    // Initialize UDNS resolver
    cout << dns_version() << endl;

    dns_init(NULL, 0);
    struct dns_ctx *ctx = dns_new(NULL);
    int sockfd = dns_open(ctx);

    cout << sockfd << endl;

    const char *hostname = "baidu.com";

    // Submit DNS query for A record
    // UDSN 异步查询
    struct dns_rr_a4 result;
    struct dns_query* query = dns_submit_a4(ctx, hostname, 0, NULL, &result);

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
    dns_ioevent(ctx, 5);
    
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

    sleep(4);

    cout << result.dnsa4_nrr << endl;

    return 0;
}
