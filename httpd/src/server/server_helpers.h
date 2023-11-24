#ifndef SERVER_HELPERS_H
#define SERVER_HELPERS_H

#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>

#include "epoll/epoll.h"
#include "http/http.h"
#include "logger/logger.h"

void accept_connections(int listen_fd, struct sockaddr *client_addrinfo,
                        int epoll_fd);
void cleanup_resources(int fd, int epoll_fd, struct response *resp,
                       struct log_info *info);
void final_cleanup(struct clients_queue *list, int fd, int epoll_fd);
struct log_info *get_logger_instance(struct config *config, struct request *req,
                                     struct sockaddr *client_addrinfo);

#endif /* SERVER_HELPERS_H */
