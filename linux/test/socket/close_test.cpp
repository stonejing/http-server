#include <sys/socket.h>
#include <map>
#include <iostream>
#include <unistd.h>

using namespace std;
int main(void)
{
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    cout << sockfd << endl;
    std::map<int, int> test;
    int tmp = sockfd;
    test[sockfd] = 8;
    close(sockfd);
    cout << sockfd << endl;
    cout << test[tmp] << endl; 

    int test_fd = socket(AF_INET, SOCK_STREAM, 0);
    std::cout << "Socket file descriptor: " << test_fd << std::endl;

    // Close the socket
    close(test_fd);
    
    // Attempt to print the closed socket file descriptor
    std::cout << "Closed socket file descriptor: " << test_fd << std::endl;

    return 0;

    return 0;
}