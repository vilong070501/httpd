#ifndef DAEMON_H
#define DAEMON_H

#include "config/config.h"

int daemonize(char *action, struct config *config);

#endif /* !DAEMON_H */
