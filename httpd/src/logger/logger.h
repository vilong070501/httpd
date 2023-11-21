#ifndef LOGGER_H
#define LOGGER_H

#include <stdio.h>

#include "config/config.h"
#include "http/http.h"
#include "utils/string.h"

struct log_info
{
    struct string *server_name;
    struct string *request_type;
    struct string *target;
    struct string *client_ip;
    int status_code;
};

struct log_info *build_log_info(struct string *name, struct request *req,
                                char *client_ip);
void set_status_code(struct log_info *info, int status_code);
void free_log_info(struct log_info *info);
void get_GMT_date_time(char *buffer, size_t size);
void log_error(FILE *log_file, const char *error);
void log_request(FILE *log_file, struct log_info *info);
void log_response(FILE *log_file, struct log_info *info);

#endif /* !LOGGER_H */
