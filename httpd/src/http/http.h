#ifndef HTTP_H
#define HTTP_H

#define SP " "
#define CRLF "\r\n"
#define DCRLF "\r\n\r\n"

#include "config/config.h"
#include "utils/string.h"

enum method
{
    UNSUPPORTED,
    GET,
    HEAD
};

struct header
{
    struct string *field_name;
    struct string *value;
    struct header *next;
};

struct request
{
    enum method method;
    struct string *target;
    struct string *version;
    struct header *headers;
};

struct response
{
    struct string *version;
    int status_code;
    struct string *reason;
    struct header *headers;
};

struct request *parse_request(const char *raw_request);
struct string *get_header(const char *header_name, struct header *headers);
void free_header(struct header *header);
void free_request(struct request *req);
struct response *build_response(struct request *req, struct config *config);
struct string *struct_response_to_string(struct response *resp);
void free_response(struct response *resp);

#endif /* !HTTP_H */
