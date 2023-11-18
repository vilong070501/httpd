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

#define PORT "4242"
#define BUFFER_LENGTH 1024

void sigchild_handler(int sig)
{
    (void)sig;
    int saved_errno = errno;
    while (waitpid(-1, NULL, WNOHANG) > 0)
        ;
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

static struct request *receive_request(int communicate_fd, int *to_read,
                                       int *bad_request)
{
    size_t request_length = 0, body_length = 0, valread;
    char *tmp;
    struct request *req;
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
            // raw_request->data[len + 4] = '\0';
            req = parse_request(raw_request->data);
            // print_request(req);
            if (!req)
            {
                *bad_request = 1;
            }
            struct string *content_length =
                get_header("Content-Length", req->headers);
            if (content_length)
            {
                content_length->data =
                    realloc(content_length->data, content_length->size + 1);
                content_length->data[content_length->size] = '\0';
                body_length = atoi(content_length->data);
                if (body_length <= 0)
                    *bad_request = 1;
                *to_read = body_length - (request_length - (len + 4));
            }
            else
            {
                *to_read = 0;
            }
            break;
        }
    }
    free(buffer);
    string_destroy(raw_request);
    return req;
}

static void build_and_send_response(struct request *req, struct config *config,
                                    int communicate_fd, int bad_request)
{
    /* Build response */
    struct response *resp = build_response(req, config);
    if (bad_request)
    {
        resp->status_code = 404;
        resp->reason = string_create("Bad Request", 11);
    }

    struct string *content_length = get_header("Content-length", resp->headers);
    char *tmp = calloc(content_length->size + 1, sizeof(char));
    memcpy(tmp, content_length->data, content_length->size);
    int file_len = atoi(tmp);
    struct string *response = struct_response_to_string(resp);
    // printf("%.*s\n", (int)response->size, response->data);

    /* Send response to client */
    send(communicate_fd, response->data, response->size, 0);

    if (resp->status_code == 200 && req->method == GET)
    {
        string_concat_str(req->target, "\0", 1);
        int file_target_fd = open(req->target->data, O_RDONLY);
        if (file_target_fd)
            sendfile(communicate_fd, file_target_fd, NULL, file_len);
    }

    free_request(req);
    free_response(resp);
    string_destroy(response);
    close(communicate_fd);
}

int start_server(struct config *config)
{
    int listen_fd, communicate_fd;
    struct sockaddr address;
    socklen_t sin_size;
    size_t valread;
    int to_read = 0, bad_request = 0;

    listen_fd = get_server_and_bind();
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

    while (1)
    {
        sin_size = sizeof(address);
        communicate_fd = accept(listen_fd, &address, &sin_size);
        if (communicate_fd == -1)
        {
            // fprintf(stderr, "accept failed\n");
            return 1;
        }

        struct request *req =
            receive_request(communicate_fd, &to_read, &bad_request);

        if (to_read > 0)
        {
            char *buffer = calloc(to_read + 1, sizeof(char));
            while (to_read > 0)
            {
                valread = recv(communicate_fd, buffer, to_read, 0);
                to_read -= valread;
            }
            free(buffer);
        }

        build_and_send_response(req, config, communicate_fd, bad_request);
    }
    close(listen_fd);
    return 0;
}
