#include "helpers.h"

#include <stdio.h>

#include "http.h"

void print_header(struct header *header)
{
    struct header *tmp = header;
    int name_len = tmp->field_name->size;
    int value_len = tmp->value->size;
    while (tmp)
    {
        printf("%.*s: %.*s\n", name_len, tmp->field_name->data, value_len,
               tmp->value->data);
        tmp = tmp->next;
    }
}

void print_method(enum method method)
{
    switch (method)
    {
    case 0:
        printf("Method: UNSUPPORTED\n");
        break;
    case 1:
        printf("Method: GET\n");
        break;
    case 2:
        printf("Method: HEAD\n");
        break;
    }
}

void print_request(struct request *req)
{
    int target_len = req->target->size;
    int version_len = req->version->size;
    print_method(req->method);
    printf("Target: %.*s\n", target_len, req->target->data);
    printf("Version: %.*s\n", version_len, req->version->data);
    printf("Headers: \n");
    print_header(req->headers);
}
