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
    if (!target)
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

int extract_headers(struct request *req, const char *raw_request)
{
    int total_len = 0;
    struct header *header = NULL, *last = NULL;
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
        header->field_name = string_create(raw_request, name_len);
        if (!header->field_name)
        {
            return -1;
        }
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
        header->value = string_create(raw_request, value_len);
        if (!header->value)
            return -1;
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

int check_missing_header(struct header *headers)
{
    if (!get_header("Host", headers))
        return 0;
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
        sprintf(path, "%s/%s", vhost->root_dir, tar);
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

int get_status_code(struct request *req, struct config *config)
{
    if (!check_method(req->method))
        return 405;
    if (!check_missing_header(req->headers))
        return 400;
    if (!check_version(req->version))
        return 505;
    int target_check = check_target(req, config);
    if (target_check == -1)
        return 404;
    else if (target_check == 0)
        return 403;
    return 200;
}

struct string *get_reason_phrase(int status_code)
{
    switch (status_code)
    {
    case 400:
        return string_create("Bad Request", 11);
    case 403:
        return string_create("Forbidden", 9);
    case 404:
        return string_create("Not Found", 9);
    case 405:
        return string_create("Method Not Allowed", 18);
    case 505:
        return string_create("HTTP Version Not Supported", 26);
    default:
        return string_create("OK", 2);
    }
}

struct string *get_header(const char *header_name, struct header *headers)
{
    struct header *tmp = headers;
    while (tmp != NULL
           && (tmp->field_name->size != strlen(header_name)
               || string_compare_n_str(tmp->field_name, header_name,
                                       tmp->field_name->size)
                   != 0))
        tmp = tmp->next;

    if (!tmp)
        return NULL;
    return tmp->value;
}
