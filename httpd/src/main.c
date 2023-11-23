#define _XOPEN_SOURCE

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#include "config/config.h"
#include "daemon/daemon.h"
#include "logger/logger.h"
#include "server/server.h"

static int dry_run;

static int check_options(int argc, char **argv, char *daemon_flag,
                         char *config_file)
{
    int c;
    opterr = 0;
    if (strcmp(argv[1], "--dry-run") == 0)
    {
        dry_run = 1;
        optind += 1;
    }

    while ((c = getopt(argc, argv, "a:")) != -1)
    {
        switch (c)
        {
        case 'a':
            strcpy(daemon_flag, optarg);
            break;
        case '?':
            if (optopt == 'a')
                fprintf(stderr, "Option -%c requires an argument.\n", optopt);
            else
                fprintf(stderr,
                        "Usage: ./httpd [--dry-run] [-a (start | stop | reload "
                        "| restart)] server.conf\n");
            return 1;
        default:
            fprintf(stderr, "Unknown option character `\\x%x'.\n", optopt);
            return 1;
        }
    }

    if (argc - optind > 1)
    {
        fprintf(stderr, "Should give only ONE configuration file\n");
        return 1;
    }
    else if (argc == optind)
    {
        fprintf(stderr, "Should provide ONE configuration file\n");
        return 1;
    }
    else
        strcpy(config_file, argv[optind]);
    return 0;
}

void free_pointers(char *daemon, char *config_file)
{
    free(daemon);
    free(config_file);
}

static int check_action(const char *daemon_flag)
{
    if (strlen(daemon_flag) != 0 && (strcmp(daemon_flag, "start") != 0)
        && (strcmp(daemon_flag, "stop") != 0)
        && (strcmp(daemon_flag, "reload") != 0)
        && (strcmp(daemon_flag, "restart") != 0))
    {
        fprintf(stderr,
                "Please choose between these 4 values for option -a: "
                "start | stop | reload | restart\n");
        return 1;
    }
    return 0;
}

FILE *get_log_file(bool log, bool daemonized, char *log_file)
{
    if (!log)
        return NULL;
    if (!daemonized)
    {
        if (!log_file)
            return stdout;
        else
            return fopen(log_file, "w+");
    }
    else
    {
        if (!log_file)
            return fopen("HTTPd.log", "w+");
        else
            return fopen(log_file, "w+");
    }
}

int main(int argc, char **argv)
{
    /* Check for options */
    if (argc < 2)
    {
        fprintf(stderr,
                "Usage: ./httpd [--dry-run] [-a (start | stop | "
                "reload | restart)] server.conf\n");
        return 1;
    }
    char *daemon_flag = calloc(1024, sizeof(char));
    char *config_file = calloc(1024, sizeof(char));

    if (check_options(argc, argv, daemon_flag, config_file) == 1)
    {
        free_pointers(daemon_flag, config_file);
        return 1;
    }

    if (check_action(daemon_flag))
    {
        free_pointers(daemon_flag, config_file);
        return 1;
    }

    struct config *config = parse_configuration(config_file);
    if (!config && dry_run)
    {
        fprintf(stderr, "Configuration file is not valid\n");
    }
    if (!config)
    {
        free_pointers(daemon_flag, config_file);
        return 2;
    }
    /* In case of dry-run */
    if (dry_run)
    {
        config_destroy(config);
        free_pointers(daemon_flag, config_file);
        return 0;
    }

    int ret = 0;
    FILE *log_file;
    if (strlen(daemon_flag) > 0)
    {
        log_file = get_log_file(config->log, true, config->log_file);
        ret = daemonize(daemon_flag, config, log_file);
    }
    else
    {
        log_file = get_log_file(config->log, false, config->log_file);
        ret = start_server(config, log_file);
    }

    if (log_file)
        fclose(log_file);
    config_destroy(config);
    free_pointers(daemon_flag, config_file);
    return ret;
}
