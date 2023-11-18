#include "logger.h"

#include <stdio.h>
#include <string.h>
#include <time.h>

#define GMT_BUFFER_LENGTH 80

void get_GMT_date_time(char *buffer, size_t size)
{
    time_t t = time(NULL);
    struct tm *tm = gmtime(&t);
    if (tm != NULL)
    {
        strftime(buffer, size, "%a, %d %b %Y %H:%M:%S GMT", tm);
    }
}

void log_error(FILE *log_file, const char *error)
{
    char error_line[1024];
    char buffer[GMT_BUFFER_LENGTH];
    get_GMT_date_time(buffer, GMT_BUFFER_LENGTH);
    sprintf(error_line, "ERROR: %s %s", buffer, error);
    fputs(error_line, log_file);
}

void log_request(FILE *log_file, const char *server_name, const char *message,
                 const char *client_ip)
{
    char error_line[1024];
    char buffer[GMT_BUFFER_LENGTH];
    get_GMT_date_time(buffer, GMT_BUFFER_LENGTH);
    sprintf(error_line, "%s [%s] received %s from %s", buffer, server_name,
            message, client_ip);
    fputs(error_line, log_file);
}

void log_response(FILE *log_file, const char *server_name, int status_code,
                  const char *client_ip_and_message)
{
    char error_line[1024];
    char buffer[GMT_BUFFER_LENGTH];
    get_GMT_date_time(buffer, GMT_BUFFER_LENGTH);
    sprintf(error_line, "%s [%s] responding with %d to %s", buffer, server_name,
            status_code, client_ip_and_message);
    fputs(error_line, log_file);
}
