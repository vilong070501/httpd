#define _XOPEN_SOURCE

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#include "config/config.h"
#include "daemon/daemon.h"
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

int main(int argc, char **argv)
{
    /* Check for options */
    if (argc < 2)
    {
        fprintf(stderr,
                "Usage: ./httpd [--dry-run] [-a (start | stop | reload "
                "| restart)] server.conf\n");
        return 1;
    }
    char *daemon_flag = calloc(1024, sizeof(char));
    char *config_file = calloc(1024, sizeof(char));

    int ret = 0;

    if ((ret = check_options(argc, argv, daemon_flag, config_file)) == 1)
    {
        free(daemon_flag);
        free(config_file);
        return 1;
    }

    if (strlen(daemon_flag) != 0 && (strcmp(daemon_flag, "start") != 0)
        && (strcmp(daemon_flag, "stop") != 0)
        && (strcmp(daemon_flag, "reload") != 0)
        && (strcmp(daemon_flag, "restart") != 0))
    {
        fprintf(stderr,
                "Please choose between these 4 values for option -a: start | "
                "stop | reload | restart\n");
        free(daemon_flag);
        free(config_file);
        return 1;
    }

    struct config *config = parse_configuration(config_file);
    /* In case of dry-run */
    if (dry_run)
    {
        if (!config)
        {
            config_destroy(config);
            fprintf(stderr, "Fail to parse configuration file %s\n",
                    config_file);
            ret = 2;
        }
        free(daemon_flag);
        free(config_file);
        return ret;
    }
    if (strlen(daemon_flag) > 0)
        ret = daemonize(daemon_flag, config->pid_file);
    else
        start_server();

    config_destroy(config);
    free(daemon_flag);
    free(config_file);
    return ret;
}
