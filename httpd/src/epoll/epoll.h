#ifndef EPOLL_H
#define EPOLL_H

#include "utils/string.h"

struct clients_queue
{
    struct string *request;
    int fd;
    struct clients_queue *next;
};

int epoll_ctl_add(int epoll_fd, int client_fd, int events);
int set_nonblocking(int sockfd);
void add_client_to_queue(struct clients_queue **list, int fd, char *req,
                         int len);
void remove_client(struct clients_queue **list, int fd);
struct clients_queue *get_client(struct clients_queue *list, int fd);
void free_queue(struct clients_queue *list);

#endif /* !EPOLL_H */
