#ifndef SERVER_H
#define SERVER_H

#include <stdio.h>

#include "config/config.h"

int start_server(struct config *config, FILE *log_file);

#endif /* !SERVER_H */
