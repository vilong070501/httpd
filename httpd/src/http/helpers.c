#include "helpers.h"

#include <stdio.h>

#include "http.h"

void print_header(struct header *header)
{
    struct header *tmp = header;
    while (tmp)
    {
        printf("%.*s: %.*s\n", (int)tmp->field_name->size,
               tmp->field_name->data, (int)tmp->value->size, tmp->value->data);
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
    print_method(req->method);
    printf("Target: %.*s\n", (int)req->target->size, req->target->data);
    printf("Version: %.*s\n", (int)req->version->size, req->version->data);
    printf("Headers: \n");
    print_header(req->headers);
}
