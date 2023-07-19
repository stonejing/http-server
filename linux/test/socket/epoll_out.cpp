#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/socket.h>

#define MAX_EVENTS 10

int main() {
    int epoll_fd, num_events;
    struct epoll_event events[MAX_EVENTS];

    // Create socket pair
    int fds[2];
    if (socketpair(AF_UNIX, SOCK_STREAM, 0, fds) == -1) {
        perror("socketpair");
        exit(1);
    }

    // Create epoll instance
    epoll_fd = epoll_create1(0);
    if (epoll_fd == -1) {
        perror("epoll_create1");
        exit(1);
    }

    // Add the socket pair to the epoll instance
    struct epoll_event event;
    event.events = EPOLLOUT;
    event.data.fd = fds[0];
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fds[0], &event) == -1) {
        perror("epoll_ctl");
        exit(1);
    }

    // Write data to the socket
    // const char* message = "Hello, epoll!";
    // if (write(fds[1], message, strlen(message)) == -1) {
    //     perror("write");
    //     exit(1);
    // }

    // Process the events
    while(true)
    {
        // Wait for events
        num_events = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
        if (num_events == -1) {
            perror("epoll_wait");
            exit(1);
        }
        for (int i = 0; i < num_events; i++) 
        {
            if (events[i].events & EPOLLOUT) {
                // EPOLLOUT event occurred on the file descriptor
                printf("EPOLLOUT event occurred on file descriptor %d\n", events[i].data.fd);
                // Handle the write operation
                // In this example, we'll simply read the data from the socket
                // char buffer[256];
                // ssize_t bytes_read = read(events[i].data.fd, buffer, 1);
                // if (bytes_read == -1) {
                //     perror("read");
                //     exit(1);
                // }
                // buffer[bytes_read] = '\0';
                // printf("Received data: %s\n", buffer);
            }
        }    
    }

    // Close file descriptors and epoll instance
    close(fds[0]);
    close(fds[1]);
    close(epoll_fd);

    return 0;
}
