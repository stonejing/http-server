#pragma once

#include "winsock2.h"
#include <vector>

int recvn(SOCKET sockfd, std::vector<char>& buffer);
int sendn(SOCKET sockfd, std::vector<char>& buffer, int buffer_len);