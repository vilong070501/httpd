#include "request.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

int extract_method(struct request *req, const char *raw_request)
{
    int method_len = strcspn(raw_request, SP);
    if (memcmp(raw_request, "GET", 3) == 0)
        req->method = GET;
    else if (strncmp(raw_request, "HEAD", 4) == 0)
        req->method = HEAD;
    else
        req->method = UNSUPPORTED;

    return method_len;
}

int extract_target(struct request *req, const char *raw_request)
{
    // TODO: How to compute this len ???
    int target_len = strcspn(raw_request, SP);
    struct string *target = string_create(raw_request, target_len);
    if (!target || target->data[0] != '/')
        return -1;
    req->target = target;
    return target_len;
}

int extract_version(struct request *req, const char *raw_request)
{
    // TODO: How to compute this len ???
    int version_len = strcspn(raw_request, CRLF);
    struct string *version = string_create(raw_request, version_len);
    if (!version)
        return -1;
    req->version = version;
    return version_len;
}

static int is_header_exist(const char *name, int len, struct header *headers)
{
    int exist = 0;
    char *tmp = calloc(len + 1, sizeof(char));
    memcpy(tmp, name, len);
    if (get_header(tmp, headers))
    {
        exist = 1;
    }
    free(tmp);
    return exist;
}

int extract_headers(struct request *req, const char *raw_request)
{
    int total_len = 0;
    struct header *header = NULL;
    struct header *last = NULL;
    while (raw_request[0] != '\r' || raw_request[1] != '\n')
    {
        last = header;
        header = calloc(1, sizeof(struct header));
        if (!header)
        {
            return -1;
        }

        // Field-name
        // TODO: This len
        int name_len = strcspn(raw_request, ":");
        if (name_len <= 0 || raw_request[name_len - 1] == '0')
        {
            return -1;
        }
        header->field_name = string_create(raw_request, name_len);
        if (!header->field_name)
        {
            return -1;
        }
        if (is_header_exist(header->field_name->data, name_len, last))
            return -1;
        raw_request += name_len + 1;
        total_len += name_len + 1;

        // Move past leading whitespaces
        while (*raw_request == ' ')
        {
            raw_request++;
            total_len++;
        }

        // Value
        // TODO: This len
        int value_len = strcspn(raw_request, CRLF);
        int value_without_space_len = 0;
        while (value_without_space_len < value_len)
        {
            if (raw_request[value_without_space_len] != ' ')
                value_without_space_len++;
            else
                break;
        }
        header->value = string_create(raw_request, value_without_space_len);
        if (!header->value)
        {
            return -1;
        }
        raw_request += value_len + 2;
        total_len += value_len + 2;

        // Next
        header->next = last;
    }
    req->headers = header;
    return total_len;
}

int check_method(enum method method)
{
    return method == UNSUPPORTED ? 0 : 1;
}

int check_version(struct string *version)
{
    char *ver = "HTTP/1.1";
    if (version->size != strlen(ver))
        return 0;
    return !string_compare_n_str(version, ver, version->size);
}

int check_missing_header(struct header *headers, struct config *config)
{
    /* Check Host field */
    struct string *host = get_header("Host", headers);
    if (!host || host->size == 0)
        return 0;
    char combined[1024] = { 0 };
    sprintf(combined, "%s:%s", config->servers->ip, config->servers->port);
    if (string_compare_n_str(host, config->servers->server_name->data,
                             host->size)
            != 0
        && string_compare_n_str(host, config->servers->ip, host->size) != 0
        && string_compare_n_str(host, combined, host->size) != 0)
        return 0;
    /* Check Content-Length field */
    struct string *content_length = get_header("Content-Length", headers);
    if (content_length)
    {
        if (string_compare_n_str(content_length, "0", 1) == 0)
            return 1;
        else
        {
            for (size_t i = 0; i < content_length->size; i++)
            {
                if (!isdigit(content_length->data[i]))
                    return 0;
            }
            struct string *tmp =
                string_create(content_length->data, content_length->size);
            string_concat_str(tmp, "\0", 1);
            if (atoi(tmp->data) <= 0)
            {
                string_destroy(tmp);
                return 0;
            }
            string_destroy(tmp);
        }
    }
    return 1;
}

int check_target(struct request *req, struct config *config)
{
    char path[1024] = { 0 };
    // TODO: Rework in case of multiple vhosts
    struct server_config *vhost = config->servers;

    if (req->target->size == 1
        && string_compare_n_str(req->target, "/", req->target->size) == 0)
        sprintf(path, "%s/%s", vhost->root_dir, vhost->default_file);
    else
    {
        char *tar = calloc(req->target->size + 1, sizeof(char));
        memcpy(tar, req->target->data, req->target->size);
        sprintf(path, "%s%s", vhost->root_dir, tar);
        free(tar);
    }

    struct stat stats;
    if (stat(path, &stats))
    {
        // File or directory does not exist
        return -1;
    }

    if (S_ISDIR(stats.st_mode))
    {
        strcat(path, "/");
        strcat(path, vhost->default_file);
    }

    if (stat(path, &stats))
        return -1;

    if (S_ISREG(stats.st_mode))
    {
        if ((stats.st_mode & S_IRUSR) != S_IRUSR)
            // Permission denied
            return 0;
    }
    string_destroy(req->target);
    // TODO: strlen(path)
    req->target = string_create(path, strlen(path));
    return 1;
}
