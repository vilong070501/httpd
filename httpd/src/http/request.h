#ifndef REQUEST_H
#define REQUEST_H

#include "http.h"
#include "utils/string.h"

int extract_method(struct request *req, const char *raw_request);
int extract_target(struct request *req, const char *raw_request);
int extract_version(struct request *req, const char *raw_request);
int extract_headers(struct request *req, const char *raw_request);
int check_method(enum method method);
int check_version(struct string *version);
int check_missing_header(struct header *headers);
int check_target(struct request *req, struct config *config);
int get_status_code(struct request *req, struct config *config);
struct string *get_reason_phrase(int status_code);
struct string *get_header(const char *header_name, struct header *headers);

#endif /* !REQUEST_H */
