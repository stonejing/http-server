#pragma once

#include <fcntl.h>
#include <signal.h>
#include <cstring>
#include <assert.h>
#include <sys/socket.h>
#include <errno.h>
#include <cstdio>

void handle_for_sigpipe();
int setnonblocking(int fd);
