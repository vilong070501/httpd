#ifndef REQUEST_H
#define REQUEST_H

#include <stddef.h>

#include "http.h"
#include "utils/string.h"

int extract_method(struct request *req, const char *raw_request, size_t len);
int extract_target(struct request *req, const char *raw_request, size_t len);
int extract_version(struct request *req, const char *raw_request, size_t len);
int extract_headers(struct request *req, const char *raw_request, size_t len);
int check_method(enum method method);
int check_version(struct string *version);
int check_missing_header(struct header *headers, struct config *config);
int check_target(struct request *req, struct config *config);

#endif /* !REQUEST_H */
