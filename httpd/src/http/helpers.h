#ifndef HELPERS_H
#define HELPERS_H

#include "http.h"

void print_request(struct request *req);
void print_method(enum method method);
void print_header(struct header *header);

#endif /* !HELPERS_H */
