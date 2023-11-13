#define _GNU_SOURCE

#include "config.h"

#include <fnmatch.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct config *parse_configuration(const char *path)
{
    if (!path)
        return NULL;

    FILE *config_file = fopen(path, "r");
    if (!config_file)
        return NULL;

    struct config *config = calloc(1, sizeof(struct config));
    if (!config)
        return NULL;

    char *line = NULL;
    size_t length = 0;
    ssize_t nread = 0;
    while ((nread = getline(&line, &length, config_file)) != -1)
    {
        if (strcmp(line, "[global]") == 0)
        {
            do
            {
                nread = getline(&line, &length, config_file);
                config->log = true;
                if (fnmatch("pid_file = *", line, FNM_NOESCAPE))
                {
                    config->pid_file = strchr(line, '=') + 1;
                }
                else if (fnmatch("log_file = *", line, FNM_NOESCAPE))
                {
                    config->log_file = strchr(line, '=') + 1;
                }
                else if (fnmatch("log = *", line, FNM_NOESCAPE))
                {
                    char *is_log = strchr(line, '=') + 1;
                    config->log = strcmp(is_log, "true") == 0 ? true : false;
                }
            } while (strcmp(line, "\n") != 0);
            if (!config->pid_file)
            {
                config_destroy(config);
                free(line);
                fclose(config_file);
                return NULL;
            }
        }
        else if (strcmp(line, "[[vhosts]]") == 0)
        {
            struct server_config vhost = { NULL };
            do
            {
                nread = getline(&line, &length, config_file);
                if (fnmatch("server_name = *", line, FNM_NOESCAPE))
                {
                    char *name = strchr(line, '=') + 1;
                    vhost.server_name = string_create(name, strlen(name));
                }
                else if (fnmatch("port = *", line, FNM_NOESCAPE))
                {
                    vhost.port = strchr(line, '=') + 1;
                }
                else if (fnmatch("ip = *", line, FNM_NOESCAPE))
                {
                    vhost.ip = strchr(line, '=') + 1;
                }
                else if (fnmatch("root_dir = *", line, FNM_NOESCAPE))
                {
                    vhost.root_dir = strchr(line, '=') + 1;
                }
                else if (fnmatch("default_file = *", line, FNM_NOESCAPE))
                {
                    vhost.default_file = strchr(line, '=') + 1;
                }
            } while (strcmp(line, "\n") != 0);

            if (!vhost.server_name || !vhost.ip || !vhost.port
                || !vhost.root_dir)
            {
                config_destroy(config);
                free(line);
                fclose(config_file);
                return NULL;
            }
            config->nb_servers++;
            config->servers =
                realloc(config->servers,
                        config->nb_servers * sizeof(struct server_config));
            config->servers[config->nb_servers - 1] = vhost;
        }
    }
    free(line);
    fclose(config_file);
    return config;
}

void config_destroy(struct config *config)
{
    for (size_t i = 0; i < config->nb_servers; i++)
    {
        struct server_config vhost = config->servers[i];
        string_destroy(vhost.server_name);
    }
    free(config->servers);
    free(config);
}
