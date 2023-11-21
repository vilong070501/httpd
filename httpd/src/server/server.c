#define _POSIX_C_SOURCE 200809L

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/sendfile.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <wait.h>

#include "config/config.h"
#include "http/helpers.h"
#include "http/http.h"
#include "http/response.h"
#include "logger/logger.h"

#define BUFFER_LENGTH 1024

static int signal_catched = 0;

void signal_handler(int sig)
{
    if (sig == SIGINT || sig == SIGTSTP || sig == SIGPIPE)
    {
        signal_catched = 1;
    }
}

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
    int request_length = 0;
    int body_length = 0;
    int valread = 0;
    char *tmp = NULL;
    struct request *req = NULL;
    struct string *raw_request = string_create("", 0);
    char *buffer = calloc(BUFFER_LENGTH, sizeof(char));
    /* Receive request from client */
    while ((valread = recv(communicate_fd, buffer, BUFFER_LENGTH, 0)))
    {
        string_concat_str(raw_request, buffer, valread);
        request_length += valread;
        if ((tmp = strstr(raw_request->data, DCRLF)))
        {
            int len = tmp - raw_request->data;
            string_concat_str(raw_request, "\0", 1);
            req = parse_request(raw_request->data);
            if (!req)
            {
                break;
            }
            struct string *content_length =
                get_header("Content-Length", req->headers);
            if (content_length)
            {
                if (string_compare_n_str(content_length, "0", 1) == 0)
                    body_length = 0;
                else
                {
                    struct string *len = string_create(content_length->data,
                                                       content_length->size);
                    string_concat_str(len, "\0", 1);
                    body_length = atoi(len->data);
                    string_destroy(len);
                }
                *to_read = body_length - (request_length - (len + 4));
            }
            break;
        }
    }
    free(buffer);
    string_destroy(raw_request);
    return req;
}

static struct response *build_and_send_response(struct request *req,
                                                struct config *config,
                                                int communicate_fd)
{
    /* Build and send response to client */
    struct response *resp = build_response(req, config);
    struct string *response = struct_response_to_string(resp);
    send(communicate_fd, response->data, response->size, 0);

    /* Send body content */
    struct string *content_length = get_header("Content-length", resp->headers);
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
    return resp;
}

static void read_body(int to_read, int communicate_fd)
{
    size_t valread;
    if (to_read > 0)
    {
        char *buffer = calloc(to_read + 1, sizeof(char));
        while (to_read > 0)
        {
            if (signal_catched)
                break;
            valread = recv(communicate_fd, buffer, to_read, 0);
            to_read -= valread;
        }
        free(buffer);
    }
}

int start_server(struct config *config, FILE *log_file)
{
    int listen_fd = -1;
    int communicate_fd = -1;
    struct sockaddr client_addrinfo;
    socklen_t sin_size = sizeof(struct sockaddr);
    struct sigaction sa = { 0 };
    int to_read = 0;

    listen_fd = get_server_and_bind(config->servers);
    if (listen_fd == -1)
    {
        // fprintf(stderr, "Server failed to bind\n");
        return 1;
    }

    if (listen(listen_fd, 10) == -1)
    {
        // fprintf(stderr, "listen failed\n");
        return 1;
    }

    sa.sa_handler = signal_handler;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTSTP, &sa, NULL);

    while (1)
    {
        communicate_fd = accept(listen_fd, &client_addrinfo, &sin_size);
        if (communicate_fd == -1)
        {
            return 1;
        }

        // If we catched a signal, break the lopp and clean up

        void *tmp = &client_addrinfo;
        struct sockaddr_in *addr = tmp;
        char client_ip[1024] = { 0 };
        inet_ntop(AF_INET, &addr->sin_addr, client_ip, INET_ADDRSTRLEN);

        struct request *req = receive_request(communicate_fd, &to_read);
        struct log_info *info =
            build_log_info(config->servers->server_name, req, client_ip);

        read_body(to_read, communicate_fd);

        struct response *resp =
            build_and_send_response(req, config, communicate_fd);
        set_status_code(info, resp->status_code);
        log_request(log_file, info);
        log_response(log_file, info);
        free_request(req);
        free_response(resp);
        free_log_info(info);
    }
    close(listen_fd);
    return 0;
}
