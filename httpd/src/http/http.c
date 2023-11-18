#define _XOPEN_SOURCE 500

#include "http.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "config/config.h"
#include "helpers.h"
#include "logger/logger.h"
#include "request.h"
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

    /* Extract request target */
    int target_len = extract_target(req, raw_request);
    if (target_len == -1)
    {
        free_request(req);
        return NULL;
    }
    raw_request += target_len + 1;

    /* Extract version */
    int version_len = extract_version(req, raw_request);
    if (version_len == -1)
    {
        free_request(req);
        return NULL;
    }
    raw_request += version_len + 2;

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

struct response *build_response(struct request *req, struct config *config)
{
    struct response *resp = calloc(1, sizeof(struct response));
    if (!req)
    {
        return NULL;
    }
    resp->version = string_create(req->version->data, req->version->size);
    resp->status_code = get_status_code(req, config);
    resp->reason = get_reason_phrase(resp->status_code);
    /* Build headers */
    // Date
    struct header *date = calloc(1, sizeof(struct header));
    date->field_name = string_create("Date", 4);
    char value[1024] = { 0 };
    get_GMT_date_time(value, 1024);
    date->value = string_create(value, strlen(value));
    // Content-length
    struct header *content_length = calloc(1, sizeof(struct header));
    content_length->field_name = string_create("Content-length", 14);
    // TODO: Compute file length with fseek
    struct string *file_name =
        string_create(req->target->data, req->target->size);
    string_concat_str(file_name, "\0", 1);
    FILE *file = fopen(file_name->data, "r");
    char file_len[1024] = { 0 };
    if (!file)
    {
        sprintf(file_len, "%d", 0);
    }
    fseek(file, 0L, SEEK_END);
    sprintf(file_len, "%ld", ftell(file));
    content_length->value = string_create(file_len, strlen(file_len));
    // Connection
    struct header *connection = calloc(1, sizeof(struct header));
    connection->field_name = string_create("Connection", 10);
    connection->value = string_create("close", 5);

    date->next = content_length;
    content_length->next = connection;

    resp->headers = date;

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
