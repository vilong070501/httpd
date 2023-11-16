#define _POSIX_C_SOURCE 200809L

#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <wait.h>

#define PORT "4242"

void sigchild_handler(int sig)
{
    (void)sig;
    int saved_errno = errno;
    while (waitpid(-1, NULL, WNOHANG) > 0);
    errno = saved_errno;
}

static int get_server_and_bind()
{
    struct addrinfo hints, *server_info, *p;
    int listen_fd;
    int yes = 1;
    int rv;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    if ((rv = getaddrinfo(NULL, PORT, &hints, &server_info)) != 0)
    {
        fprintf(stderr, "getaddrinfo failed\n");
        return 1;
    }

    // Loop through all the results and bind to the first that works
    for (p = server_info; p; p = p->ai_next)
    {
        if ((listen_fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol))
            == -1)
            continue;

        if (setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes))
            == -1)
        {
            fprintf(stderr, "setsockopt failed\n");
            return 1;
        }

        if (bind(listen_fd, p->ai_addr, p->ai_addrlen) == -1)
        {
            close(listen_fd);
            continue;
        }

        break;
    }

    freeaddrinfo(server_info);

    if (!p)
        return -1;

    return listen_fd;
}

int start_server()
{
    int listen_fd, communicate_fd;
    struct sockaddr_in address;
    socklen_t sin_size;
    struct sigaction sa;
    char buffer[1024];
    ssize_t valread;

    listen_fd = get_server_and_bind();
    if (listen_fd == -1)
    {
        fprintf(stderr, "Server failed to bind\n");
        return 1;
    }

    if (listen(listen_fd, 10) == -1)
    {
        fprintf(stderr, "listen failed\n");
        return 1;
    }

    sa.sa_handler = sigchild_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if (sigaction(SIGCHLD, &sa, NULL) == -1)
    {
        fprintf(stderr, "sigaction failed\n");
        return 1;
    }

    while (1)
    {
        sin_size = sizeof(address);
        communicate_fd =
            accept(listen_fd, (struct sockaddr *)&address, &sin_size);
        if (communicate_fd == -1)
        {
            fprintf(stderr, "accept failed\n");
            return 1;
        }
        printf("New client connected\n");

        while (
            (valread = recv(communicate_fd, buffer, sizeof(buffer) - 1, 0)))
        {
            if (send(communicate_fd, buffer, valread, 0) == -1)
                fprintf(stderr, "send failed\n");
            memset(buffer, 0, 1024);
        }
        printf("Client disconnected\n");
        close(communicate_fd);
    }
    close(listen_fd);
    return 0;
}
