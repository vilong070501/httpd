#include "logger.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define ERROR_BUFFER_LENGTH 1024
#define GMT_BUFFER_LENGTH 80

#include "http/http.h"

struct log_info *build_log_info(struct string *name, struct request *req,
                                char *client_ip)
{
    struct log_info *info = calloc(1, sizeof(struct log_info));
    if (!info)
        return NULL;

    info->server_name = string_create(name->data, name->size);
    string_concat_str(info->server_name, "\0", 1);
    info->client_ip = string_create(client_ip, strlen(client_ip));
    string_concat_str(info->client_ip, "\0", 1);
    if (!req)
    {
        info->status_code = 400;
        return info;
    }
    switch (req->method)
    {
    case GET:
        info->request_type = string_create("GET", 3);
        break;
    case HEAD:
        info->request_type = string_create("HEAD", 4);
        break;
    case UNSUPPORTED:
        info->request_type = string_create("UNKNOWN", 7);
        break;
    }
    string_concat_str(info->request_type, "\0", 1);
    info->target = string_create(req->target->data, req->target->size);
    string_concat_str(info->target, "\0", 1);
    return info;
}

void set_status_code(struct log_info *info, int status_code)
{
    info->status_code = status_code;
}

void free_log_info(struct log_info *info)
{
    string_destroy(info->server_name);
    string_destroy(info->request_type);
    string_destroy(info->target);
    string_destroy(info->client_ip);
    free(info);
}

void get_GMT_date_time(char *buffer, size_t size)
{
    time_t t = time(NULL);
    struct tm *tm = gmtime(&t);
    if (tm != NULL)
    {
        strftime(buffer, size, "%a, %d %b %Y %H:%M:%S GMT", tm);
    }
}

void log_error(FILE *log_file, const char *error)
{
    if (!log_file || !error)
        return;
    char error_buffer[ERROR_BUFFER_LENGTH];
    char date[GMT_BUFFER_LENGTH];
    get_GMT_date_time(date, GMT_BUFFER_LENGTH);
    sprintf(error_buffer, "ERROR: %s %s", date, error);
    fputs(error_buffer, log_file);
}

void log_request(FILE *log_file, struct log_info *info)
{
    if (!log_file || !info)
        return;
    char request_buffer[ERROR_BUFFER_LENGTH];
    char date[GMT_BUFFER_LENGTH];
    int length;
    get_GMT_date_time(date, GMT_BUFFER_LENGTH);
    if (info->status_code == 400)
    {
        length =
            sprintf(request_buffer, "%s [%s] received Bad Request from %s\n",
                    date, info->server_name->data, info->client_ip->data);
    }
    else
    {
        length =
            sprintf(request_buffer, "%s [%s] received %s on '%s' from %s\n",
                    date, info->server_name->data, info->request_type->data,
                    info->target->data, info->client_ip->data);
    }
    fwrite(request_buffer, length, sizeof(char), log_file);
    fflush(log_file);
}

void log_response(FILE *log_file, struct log_info *info)
{
    if (!log_file || !info)
        return;
    char response_buffer[ERROR_BUFFER_LENGTH];
    char date[GMT_BUFFER_LENGTH];
    int length;
    get_GMT_date_time(date, GMT_BUFFER_LENGTH);
    if (info->status_code == 400)
    {
        length = sprintf(response_buffer, "%s [%s] responding with %d to %s\n",
                         date, info->server_name->data, info->status_code,
                         info->client_ip->data);
    }
    else
    {
        length = sprintf(response_buffer,
                         "%s [%s] responding with %d to %s for %s on '%s'\n",
                         date, info->server_name->data, info->status_code,
                         info->client_ip->data, info->request_type->data,
                         info->target->data);
    }
    fwrite(response_buffer, length, sizeof(char), log_file);
    fflush(log_file);
}
