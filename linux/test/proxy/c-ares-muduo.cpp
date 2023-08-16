#include <ares.h>
#include <stdio.h>
#include <stdlib.h>
#include <cstring>
#include <string>

void ares_sock_state_callback_t(void *data, ares_socket_t socket_fd, int readable, int writable)
{
    printf("socket state callback\n");
}

int ares_sock_create_callback_t(int socket_fd, int type, void* data)
{
    printf("socket create callback\n");
}

int main(void)
{
    ares_channel channel = NULL;

    struct ares_options options;
    int optmask = ARES_OPT_FLAGS;

    options.flags = ARES_FLAG_NOCHECKRESP;
    options.flags |= ARES_FLAG_STAYOPEN;
    options.flags |= ARES_FLAG_IGNTC;
    optmask |= ARES_OPT_SOCK_STATE_CB;
    options.sock_state_cb = ares_sock_state_callback_t;
    optmask |= ARES_OPT_TIMEOUT;
    options.timeout = 2;

    int status = ares_init_options(&channel, &options, optmask);
    if (status != ARES_SUCCESS)
    {
        printf("ares_init_options failed\n");
        return -1;
    }

    ares_set_socket_callback(channel, ares_sock_create_callback_t, NULL);

    
}