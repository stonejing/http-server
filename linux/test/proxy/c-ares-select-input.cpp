#include <arpa/inet.h>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ares.h>
#include <sys/select.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ares.h>
#include <sys/select.h>

#define MAX_FDS 10

// Callback function for c-ares to process query results
void ares_query_callback(void *arg, int status, int timeouts, struct hostent *hostent) {
    if (status == ARES_SUCCESS) {
        struct in_addr **addr_list = (struct in_addr **)hostent->h_addr_list;
        for (int i = 0; addr_list[i] != NULL; ++i) {
            printf("IP Address %d: %s\n", i + 1, inet_ntoa(*addr_list[i]));
        }
    } else {
        fprintf(stderr, "ares_query_callback() failed: %s\n", ares_strerror(status));
    }

    // // Clean up
    // ares_free_hostent(hostent);
    // ares_cancel((ares_channel)arg);
}

// Callback function to handle c-ares socket state changes
void ares_sock_state_cb_1(ares_socket_t socket_fd, int readable, void*data) 
{
    if (readable) {
        fd_set *read_fds = (fd_set *)data;
        FD_SET(socket_fd, read_fds);
    }
}

int main() {
    ares_channel channel;
    int status;
    int nfds = 0;
    fd_set read_fds;
    struct timeval timeout;
    int nfds_ready;

    status = ares_init(&channel);
    if (status != ARES_SUCCESS) {
        fprintf(stderr, "ares_init() failed: %s\n", ares_strerror(status));
        return 1;
    }

    // printf("Enter a hostname: ");
    // fflush(stdout);

    while (1) {
        FD_ZERO(&read_fds);
        nfds = 0;
        // Monitor c-ares internal sockets for readability
        ares_socket_t socks[ARES_GETSOCK_MAXNUM];
        int bitmask = ares_getsock(channel, socks, ARES_GETSOCK_MAXNUM);
        for (int i = 0; i < ARES_GETSOCK_MAXNUM; ++i) {
            if (ARES_GETSOCK_READABLE(bitmask, i)) {
                FD_SET(socks[i], &read_fds);
                nfds = (nfds > socks[i]) ? nfds : socks[i];
            }
        }
        // Monitor stdin for readability
        FD_SET(STDIN_FILENO, &read_fds);
        nfds = (nfds > STDIN_FILENO) ? nfds : STDIN_FILENO;

        // Set timeout for select()
        timeout.tv_sec = 0;
        timeout.tv_usec = 0;

        // Wait for input using select()
        nfds_ready = select(nfds + 1, &read_fds, NULL, NULL, &timeout);
        if (nfds_ready < 0) {
            perror("select");
            break;
        }

        // Process input
        if (FD_ISSET(STDIN_FILENO, &read_fds)) {
            char input_buffer[256];
            if (fgets(input_buffer, sizeof(input_buffer), stdin) == NULL) {
                fprintf(stderr, "Error reading input.\n");
                break;
            }

            // Remove newline character from input
            input_buffer[strcspn(input_buffer, "\n")] = '\0';

            // Perform DNS query using c-ares
            ares_gethostbyname(channel, input_buffer, AF_INET, ares_query_callback, channel);
        }
        else {
            // Process c-ares socket activity
            ares_process(channel, &read_fds, NULL);
        }
    }

    // Clean up
    ares_destroy(channel);

    return 0;
}
