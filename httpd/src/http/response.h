#ifndef RESPONSE_H
#define RESPONSE_H

#include "http.h"

struct header *build_response_headers(int content_length);
struct response *build_bad_request_response();

#endif /* !RESPONSE_H */
