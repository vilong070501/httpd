#ifndef LOGGER_H
#define LOGGER_H

#include <stdio.h>

void get_GMT_date_time(char *buffer, size_t size);
void log_error(FILE *log_file, const char *error);
void log_request(FILE *log_file, const char *server_name, const char *message,
                 const char *client_ip);
void log_response(FILE *log_file, const char *server_name, int status_code,
                  const char *client_ip_and_message);

#endif /* !LOGGER_H */
