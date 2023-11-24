#include "epoll.h"

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/epoll.h>
#include <unistd.h>

#include "utils/string.h"

int epoll_ctl_add(int epoll_fd, int client_fd, int events)
{
    struct epoll_event event;
    event.events = events;
    event.data.fd = client_fd;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &event) == -1)
    {
        perror("Error: epoll_ctl\n");
        return -1;
    }
    return 0;
}

int set_nonblocking(int sockfd)
{
    if (fcntl(sockfd, F_SETFL, fcntl(sockfd, F_GETFL, 0) | O_NONBLOCK) == -1)
    {
        return -1;
    }
    return 0;
}

void add_client_to_queue(struct clients_queue **list, int fd, char *req,
                         int len)
{
    struct clients_queue *new = calloc(1, sizeof(struct clients_queue));
    new->fd = fd;
    if (!req)
        new->request = string_create("", 0);
    else
        new->request = string_create(req, len);
    if (!*list)
    {
        *list = new;
    }
    else
    {
        struct clients_queue *tmp = *list;
        while (tmp->next)
            tmp = tmp->next;
        tmp->next = new;
    }
}

struct clients_queue *get_client(struct clients_queue *list, int fd)
{
    if (!list)
        return NULL;
    struct clients_queue *tmp = list;
    while (tmp != NULL && tmp->fd != fd)
        tmp = tmp->next;
    if (!tmp)
        return NULL;
    return tmp;
}

void free_queue(struct clients_queue *list)
{
    if (!list)
        return;

    free_queue(list->next);
    string_destroy(list->request);
    free(list);
}

void remove_client(struct clients_queue **list, int fd)
{
    if (!*list)
        return;
    if ((*list)->fd == fd)
    {
        free_queue(*list);
        *list = NULL;
    }
    else
    {
        struct clients_queue *tmp = (*list)->next;
        while (tmp != NULL && tmp->next != NULL && tmp->next->fd != fd)
            tmp = tmp->next;

        if (tmp->next != NULL && tmp->next->fd == fd)
        {
            struct clients_queue *delete = tmp->next;
            tmp->next = tmp->next->next;
            free_queue(delete);
        }
    }
}
