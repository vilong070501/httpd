#define _XOPEN_SOURCE 500

#include "http.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "config/config.h"
#include "helpers.h"
#include "logger/logger.h"
#include "request.h"
#include "response.h"
#include "utils/string.h"

struct request *parse_request(const char *raw_request)
{
    struct request *req = calloc(1, sizeof(struct request));
    if (!req)
        return NULL;

    memset(req, 0, sizeof(struct request));

    /*  Extract method */
    int method_len = extract_method(req, raw_request);
    raw_request += method_len + 1;
    if (*raw_request == ' ')
        return NULL;

    /* Extract request target */
    int target_len = extract_target(req, raw_request);
    if (target_len == -1)
    {
        free_request(req);
        return NULL;
    }
    raw_request += target_len + 1;
    if (*raw_request == ' ')
        return NULL;

    /* Extract version */
    int version_len = extract_version(req, raw_request);
    if (version_len == -1)
    {
        free_request(req);
        return NULL;
    }
    raw_request += version_len + 2;
    if (*raw_request == ' ')
        return NULL;

    /* Extract headers */
    int headers_len = extract_headers(req, raw_request);
    if (headers_len == -1)
    {
        free_request(req);
        return NULL;
    }
    raw_request += headers_len + 2;

    return req;
}

void free_header(struct header *header)
{
    if (!header)
        return;

    string_destroy(header->field_name);
    string_destroy(header->value);
    free_header(header->next);
    free(header);
}

void free_request(struct request *req)
{
    if (!req)
        return;

    string_destroy(req->target);
    string_destroy(req->version);
    free_header(req->headers);
    free(req);
}

static int get_status_code(struct request *req, struct config *config)
{
    if (!check_method(req->method))
        return 405;
    if (!check_missing_header(req->headers, config))
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

static struct string *get_reason_phrase(int status_code)
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

struct response *build_response(struct request *req, struct config *config)
{
    // Bad Request
    if (!req)
    {
        return build_bad_request_response();
    }
    struct response *resp = calloc(1, sizeof(struct response));
    if (!resp)
    {
        return NULL;
    }
    resp->version = string_create("HTTP/1.1", 8);
    resp->status_code = get_status_code(req, config);
    resp->reason = get_reason_phrase(resp->status_code);
    /* Build headers */
    int content_length = 0;
    if (req->method == GET)
    {
        string_concat_str(req->target, "\0", 1);
        if (resp->status_code == 200)
        {
            FILE *file = fopen(req->target->data, "r");
            if (file)
            {
                fseek(file, 0L, SEEK_END);
                content_length = ftell(file);
                fclose(file);
            }
        }
    }

    resp->headers = build_response_headers(content_length);

    return resp;
}

struct string *struct_response_to_string(struct response *resp)
{
    // Calculate the length of the string needed
    size_t length = resp->version->size + resp->reason->size + 7;
    for (struct header *tmp = resp->headers; tmp; tmp = tmp->next)
    {
        // Key: Value\r\n
        length += tmp->field_name->size + tmp->value->size + 4;
    }
    length += 2; // The headers-ending CLRF

    // Format the string
    struct string *result = string_create("", 0);
    if (!result)
        return NULL;
    string_concat_str(result, resp->version->data, resp->version->size);
    string_concat_str(result, " ", 1);
    char status[4];
    sprintf(status, "%d", resp->status_code);
    string_concat_str(result, status, 3);
    string_concat_str(result, " ", 1);
    string_concat_str(result, resp->reason->data, resp->reason->size);
    string_concat_str(result, "\r\n", 2);

    for (struct header *tmp = resp->headers; tmp; tmp = tmp->next)
    {
        string_concat_str(result, tmp->field_name->data, tmp->field_name->size);
        string_concat_str(result, ": ", 2);
        string_concat_str(result, tmp->value->data, tmp->value->size);
        string_concat_str(result, "\r\n", 2);
    }
    string_concat_str(result, "\r\n", 2);

    return result;
}

void free_response(struct response *resp)
{
    if (!resp)
        return;

    string_destroy(resp->version);
    string_destroy(resp->reason);
    free_header(resp->headers);
    free(resp);
}
