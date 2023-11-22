#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define MAX_EVENTS 10
#define READ_SIZE 10

int main(int argc, char **argv)
{
    struct epoll_event event;
    struct epoll_event events_array[MAX_EVENTS];
    int event_count;
    ssize_t valread;
    char buffer[READ_SIZE + 1];

    if (argc != 2)
    {
        printf("Bad usage: ./epoll <pipe_name>\n");
        return 1;
    }

    int fd = open(argv[1], O_RDONLY | O_NONBLOCK);
    int epoll_fd = epoll_create1(0);
    event.data.fd = fd;
    event.events = EPOLLIN | EPOLLET;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, &event))
    {
        close(fd);
        close(epoll_fd);
        return 1;
    }
    int running = 1;
    while (running)
    {
        if ((event_count = epoll_wait(epoll_fd, events_array, MAX_EVENTS, -1))
            < 0)
        {
            break;
        }

        for (int i = 0; i < event_count; i++)
        {
            valread = read(fd, buffer, READ_SIZE);

            if (valread > 0)
            {
                buffer[valread] = '\0';
                if (strcmp(buffer, "ping") == 0)
                    printf("pong!\n");
                else if (strcmp(buffer, "pong") == 0)
                    printf("ping!\n");
                else if (strcmp(buffer, "quit") == 0)
                {
                    printf("quit\n");
                    running = 0;
                }
                else
                    printf("Unknown: %s\n", buffer);
                epoll_ctl(epoll_fd, EPOLL_CTL_MOD, fd, &event);
            }
        }
    }

    close(fd);
    close(epoll_fd);
    return 0;
}
