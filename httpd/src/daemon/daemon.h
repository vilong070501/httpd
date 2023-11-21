#ifndef DAEMON_H
#define DAEMON_H

#include <stdio.h>

#include "config/config.h"

int daemonize(char *action, struct config *config, FILE *log_file);

#endif /* !DAEMON_H */
