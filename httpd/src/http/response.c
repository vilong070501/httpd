#include "response.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "logger/logger.h"

struct header *build_response_headers(int cl)
{
    // Date
    struct header *date = calloc(1, sizeof(struct header));
    date->field_name = string_create("date", 4);
    char value[1024] = { 0 };
    get_GMT_date_time(value, 1024);
    date->value = string_create(value, strlen(value));
    // Content-length
    struct header *content_length = calloc(1, sizeof(struct header));
    content_length->field_name = string_create("content-length", 14);
    char content_len[1024] = { 0 };
    sprintf(content_len, "%d", cl);
    content_length->value = string_create(content_len, strlen(content_len));
    // Connection
    struct header *connection = calloc(1, sizeof(struct header));
    connection->field_name = string_create("connection", 10);
    connection->value = string_create("close", 5);

    content_length->next = connection;
    date->next = content_length;
    return date;
}

struct response *build_bad_request_response(void)
{
    struct response *resp = calloc(1, sizeof(struct response));
    if (!resp)
    {
        return NULL;
    }
    resp->version = string_create("HTTP/1.1", 8);
    resp->status_code = 400;
    resp->reason = string_create("Bad Request", 11);
    resp->headers = build_response_headers(0);
    return resp;
}
