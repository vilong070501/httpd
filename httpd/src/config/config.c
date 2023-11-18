#define _GNU_SOURCE

#include "config.h"

#include <fnmatch.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void print_config(struct config *config)
{
    if (!config)
    {
        printf("Configuration is NULL\n");
        return;
    }
    printf("[global]\n");
    printf("pid_file = %s\n", config->pid_file);
    printf("log_file = %s\n", config->log_file);
    printf("log = %d\n", config->log);

    struct server_config *servers = config->servers;

    for (size_t i = 0; i < config->nb_servers; i++)
    {
        if (i < config->nb_servers)
            printf("\n");
        printf("[[vhosts]]\n");
        // printf("server_name = %.*s\n", (int)servers[i].server_name->size,
        //        servers[i].server_name->data);
        printf("port = %s\n", servers[i].port);
        printf("ip = %s\n", servers[i].ip);
        printf("root_dir = %s\n", servers[i].root_dir);
        printf("default_file = %s\n", servers[i].default_file);
    }
}

char *extract_field(char *line)
{
    char *field = strchr(line, '=') + 2;
    size_t len = strlen(field);
    char *registered = malloc(len);
    strncpy(registered, field, len - 1);
    registered[len - 1] = '\0';
    return registered;
}

int parse_global_section(struct config *config, FILE *config_file)
{
    char *line = NULL;
    size_t length = 0;
    ssize_t nread = 0;

    while ((nread = getline(&line, &length, config_file)) != -1)
    {
        if (fnmatch("pid_file = *", line, FNM_NOESCAPE) == 0)
        {
            config->pid_file = extract_field(line);
        }
        else if (fnmatch("log_file = *", line, FNM_NOESCAPE) == 0)
        {
            config->log_file = extract_field(line);
        }
        else if (fnmatch("log = *", line, FNM_NOESCAPE) == 0)
        {
            char *is_log = strchr(line, '=') + 2;
            config->log = strcmp(is_log, "true\n") == 0 ? true : false;
        }
        else if (strcmp(line, "\n") == 0)
        {
            break;
        }
    }

    if (!config->pid_file)
    {
        config_destroy(config);
        free(line);
        fclose(config_file);
        return -1;
    }

    free(line);

    return 0;
}

int parse_vhosts_section(struct config *config, FILE *config_file)
{
    char *line = NULL;
    size_t length = 0;
    ssize_t nread = 0;

    config->nb_servers++;
    config->servers = realloc(
        config->servers, config->nb_servers * sizeof(struct server_config));
    struct server_config *vhost = config->servers + config->nb_servers - 1;
    vhost->server_name = NULL;
    vhost->port = NULL;
    vhost->ip = NULL;
    vhost->root_dir = NULL;
    vhost->default_file = NULL;
    while ((nread = getline(&line, &length, config_file)) != -1)
    {
        if (fnmatch("server_name = *", line, FNM_NOESCAPE) == 0)
        {
            char *name = strchr(line, '=') + 2;
            config->servers[config->nb_servers - 1].server_name =
                string_create(name, strlen(name) - 1);
        }
        else if (fnmatch("port = *", line, FNM_NOESCAPE) == 0)
        {
            config->servers[config->nb_servers - 1].port = extract_field(line);
        }
        else if (fnmatch("ip = *", line, FNM_NOESCAPE) == 0)
        {
            config->servers[config->nb_servers - 1].ip = extract_field(line);
        }
        else if (fnmatch("root_dir = *", line, FNM_NOESCAPE) == 0)
        {
            config->servers[config->nb_servers - 1].root_dir =
                extract_field(line);
        }
        else if (fnmatch("default_file = *", line, FNM_NOESCAPE) == 0)
        {
            config->servers[config->nb_servers - 1].default_file =
                extract_field(line);
        }
        else if (strcmp(line, "\n") == 0)
        {
            break;
        }
    }

    if (!vhost->server_name || !vhost->ip || !vhost->port || !vhost->root_dir)
    {
        config_destroy(config);
        free(line);
        fclose(config_file);
        return -1;
    }
    if (!vhost->default_file)
    {
        vhost->default_file = strdup("index.html");
    }

    free(line);

    return 0;
}

struct config *parse_configuration(const char *path)
{
    if (!path)
        return NULL;

    FILE *config_file = fopen(path, "r");
    if (!config_file)
        return NULL;

    struct config *config = malloc(sizeof(struct config));
    config->pid_file = NULL;
    config->log_file = NULL;
    config->log = true;
    config->servers = NULL;
    config->nb_servers = 0;
    if (!config)
        return NULL;

    char *line = NULL;
    size_t length = 0;
    ssize_t nread = 0;
    while ((nread = getline(&line, &length, config_file)) != -1)
    {
        if (strcmp(line, "[global]\n") == 0)
        {
            if (parse_global_section(config, config_file) == -1)
            {
                free(line);
                return NULL;
            }
        }
        else if (strcmp(line, "[[vhosts]]\n") == 0)
        {
            if (parse_vhosts_section(config, config_file) == -1)
            {
                free(line);
                return NULL;
            }
        }
    }
    free(line);
    fclose(config_file);
    if (config->nb_servers == 0)
    {
        config_destroy(config);
        return NULL;
    }

    return config;
}

void config_destroy(struct config *config)
{
    struct server_config *servers = config->servers;
    for (size_t i = 0; i < config->nb_servers; i++)
    {
        struct server_config vhost = servers[i];
        string_destroy(vhost.server_name);
        free(vhost.port);
        free(vhost.ip);
        free(vhost.root_dir);
        free(vhost.default_file);
    }
    free(config->pid_file);
    free(config->log_file);
    free(config->servers);
    free(config);
}
