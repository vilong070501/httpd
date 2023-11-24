#define _POSIX_C_SOURCE 200809L

#include "server_helpers.h"

#include <arpa/inet.h>
#include <fcntl.h>
#include <netdb.h>
#include <signal.h>
#include <stdio.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "config/config.h"
#include "epoll/epoll.h"
#include "http/http.h"
#include "logger/logger.h"

void accept_connections(int listen_fd, struct sockaddr *client_addrinfo,
                        int epoll_fd)
{
    socklen_t sin_size = sizeof(struct sockaddr);
    int communicate_fd = accept(listen_fd, client_addrinfo, &sin_size);
    set_nonblocking(communicate_fd);
    epoll_ctl_add(epoll_fd, communicate_fd,
                  EPOLLIN | EPOLLET | EPOLLRDHUP | EPOLLHUP);
}

void cleanup_resources(int fd, int epoll_fd, struct response *resp,
                       struct log_info *info)
{
    free_response(resp);
    free_log_info(info);
    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, NULL);
}

void final_cleanup(struct clients_queue *list, int fd, int epoll_fd)
{
    free_queue(list);
    close(epoll_fd);
    close(fd);
}

struct log_info *get_logger_instance(struct config *config, struct request *req,
                                     struct sockaddr *client_addrinfo)
{
    void *tmp = client_addrinfo;
    struct sockaddr_in *addr = tmp;
    char client_ip[1024] = { 0 };
    inet_ntop(AF_INET, &addr->sin_addr, client_ip, INET_ADDRSTRLEN);

    struct log_info *info =
        build_log_info(config->servers->server_name, req, client_ip);
    return info;
}
