#define _POSIX_C_SOURCE 200809L

#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/sendfile.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "config/config.h"
#include "epoll/epoll.h"
#include "http/helpers.h"
#include "http/http.h"
#include "http/response.h"
#include "logger/logger.h"
#include "server_helpers.h"

#define BUFFER_LENGTH 1024
#define MAX_EVENTS 10

static int signal_catched = 0;
static struct clients_queue *list = NULL;

static int get_server_and_bind(struct server_config *servers)
{
    struct addrinfo hints;
    struct addrinfo *server_info = NULL;
    struct addrinfo *p = NULL;
    int listen_fd = -1;
    int yes = 1;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    /* TODO: Rework this when dealing with multiple vhost */
    if ((getaddrinfo(NULL, servers->port, &hints, &server_info)) != 0)
    {
        // fprintf(stderr, "getaddrinfo failed\n");
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
            // fprintf(stderr, "setsockopt failed\n");
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

static struct request *receive_request(int communicate_fd, int *to_read)
{
    int body_length = 0;
    int valread = 0;
    char *tmp = NULL;
    struct request *req = NULL;
    char *buffer = calloc(BUFFER_LENGTH, sizeof(char));
    /* Receive request from client */
    valread = recv(communicate_fd, buffer, BUFFER_LENGTH, 0);
    struct clients_queue *client = get_client(list, communicate_fd);
    if (!client)
        add_client_to_queue(&list, communicate_fd, buffer, valread);
    else
    {
        string_concat_str(client->request, buffer, valread);
    }

    client = get_client(list, communicate_fd);
    if (!client)
    {
        free(buffer);
        return req;
    }

    if ((tmp = string_strstr(client->request, DCRLF)))
    {
        int len = tmp - client->request->data;
        req = parse_request(client->request->data, client->request->size);
        if (!req)
        {
            free(buffer);
            return req;
        }
        struct string *content_length =
            get_header("Content-Length", req->headers);
        if (content_length)
        {
            if (string_compare_n_str(content_length, "0", 1) == 0)
                body_length = 0;
            else
            {
                struct string *length =
                    string_create(content_length->data, content_length->size);
                string_concat_str(length, "\0", 1);
                body_length = atoi(length->data);
                string_destroy(length);
            }
            *to_read = body_length - (client->request->size - (len + 4));
        }
    }

    free(buffer);
    return req;
}

static void build_and_send_response(struct request *req, struct response *resp,
                                    int communicate_fd)
{
    /* Build and send response to client */
    struct string *response = struct_response_to_string(resp);
    send(communicate_fd, response->data, response->size, 0);

    /* Send body content */
    struct string *content_length = get_header("content-length", resp->headers);
    string_concat_str(content_length, "\0", 1);
    int file_len = atoi(content_length->data);
    if (file_len > 0 && resp->status_code == 200 && req->method == GET)
    {
        string_concat_str(req->target, "\0", 1);
        int file_target_fd = open(req->target->data, O_RDONLY);
        if (file_target_fd)
            sendfile(communicate_fd, file_target_fd, NULL, file_len);
    }

    string_destroy(response);
    close(communicate_fd);
}

static void read_body(int *to_read, int communicate_fd)
{
    int valread;
    if (*to_read > 0)
    {
        char *buffer = calloc(*to_read + 1, sizeof(char));
        valread = recv(communicate_fd, buffer, *to_read, 0);
        if (valread > 0)
        {
            *to_read -= valread;
        }
        free(buffer);
    }
}

int client_hangup(int fd, int epoll_fd, int events)
{
    if (events & (EPOLLRDHUP | EPOLLHUP))
    {
        // printf("Client connection closed\n");
        epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, NULL);
        close(fd);
        return 1;
    }
    return 0;
}

void signal_handler(int sig)
{
    if (sig == SIGINT || sig == SIGTSTP)
    {
        signal_catched = 1;
    }
}

static void handle_signal(void)
{
    struct sigaction sa = { 0 };
    sa.sa_handler = signal_handler;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTSTP, &sa, NULL);
}

int setup_epoll(int server_fd)
{
    set_nonblocking(server_fd);

    if (listen(server_fd, 10) == -1)
    {
        // fprintf(stderr, "listen failed\n");
        return -1;
    }

    handle_signal();

    int epoll_fd = epoll_create1(0);
    if (epoll_ctl_add(epoll_fd, server_fd, EPOLLIN) == -1)
    {
        close(server_fd);
        close(epoll_fd);
        return -1;
    }
    return epoll_fd;
}

int start_server(struct config *config, FILE *log_file)
{
    struct sockaddr client_addrinfo;
    struct epoll_event events_array[MAX_EVENTS];
    int to_read = 0;

    int listen_fd = get_server_and_bind(config->servers);
    if (listen_fd == -1)
    {
        // fprintf(stderr, "Server failed to bind\n");
        return 1;
    }

    int epoll_fd = setup_epoll(listen_fd);
    if (epoll_fd == -1)
        return 1;

    while (1)
    {
        int nfds = epoll_wait(epoll_fd, events_array, MAX_EVENTS, -1);

        for (int i = 0; i < nfds; i++)
        {
            if (events_array[i].events == EPOLLIN)
            {
                int event_fd = events_array[i].data.fd;
                if (event_fd == listen_fd)
                {
                    accept_connections(listen_fd, &client_addrinfo, epoll_fd);
                }
                else
                {
                    struct request *req = receive_request(event_fd, &to_read);

                    /* Read the incoming body */
                    read_body(&to_read, event_fd);

                    struct clients_queue *client = get_client(list, event_fd);
                    if (client != NULL && string_strstr(client->request, DCRLF)
                        && to_read <= 0)
                    {
                        /* Logging and send response */
                        struct log_info *info =
                            get_logger_instance(config, req, &client_addrinfo);
                        /* Log complete request */
                        log_request(log_file, info);

                        /* Build response */
                        struct response *resp = build_response(req, config);
                        set_status_code(info, resp->status_code);

                        /* Log response */
                        log_response(log_file, info);

                        /* Build and send response to client */
                        build_and_send_response(req, resp, event_fd);

                        remove_client(&list, event_fd);
                        cleanup_resources(event_fd, epoll_fd, resp, info);
                    }
                    free_request(req);
                }
            }
            if (client_hangup(events_array[i].data.fd, epoll_fd,
                              events_array[i].events))
            {
                continue;
            }
        }
        // SIGINT or SIGTSTP catched
        if (signal_catched == 1)
            break;
    }
    final_cleanup(list, listen_fd, epoll_fd);
    return 0;
}
