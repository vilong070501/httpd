#define _GNU_SOURCE

#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#include "daemon.h"
#include "server/server.h"

static int file_exists(const char *path)
{
    FILE *file = fopen(path, "r");
    int exist = 1;
    if (!file && errno == ENOENT)
        exist = 0;
    else
        fclose(file);
    return exist;
}

static int process_alive(pid_t pid)
{
    return kill(pid, 0) == 0 ? 1 : 0;
}

static int get_pid_from_file(FILE *pid_file)
{
    char *line = NULL;
    size_t len = 0;
    getline(&line, &len, pid_file);
    int pid = atoi(line);
    free(line);
    return pid;
}

void sigint_handler(int sig)
{
    (void)sig;
    _exit(EXIT_SUCCESS);
}

static int start_daemon(const char *pid_file_path, int exist)
{
    FILE *pid_file;
    if (exist)
    {
        pid_file = fopen(pid_file_path, "r");
        int old_pid = get_pid_from_file(pid_file);
        fclose(pid_file);
        /* Check if old_pid is alive */
        // If it is, stop the program and return 1
        if (process_alive(old_pid))
            return 1;
    }

    /* Start forking */
    pid_t pid = fork();
    if (!pid)
    {
        struct sigaction sa;
        sa.sa_handler = sigint_handler;
        sigemptyset(&sa.sa_mask);
        sa.sa_flags = SA_RESTART;
        if (sigaction(SIGINT, &sa, NULL) == -1)
        {
            fprintf(stderr, "Daemon: sigaction failed\n");
            return 1;
        }
        /* Child process: start the server */
        return start_server();
    }
    else
    {
        /* Parent process: write PID to pid_file */
        char str[16] = { 0 };
        sprintf(str, "%d\n", pid);
        pid_file = fopen(pid_file_path, "w");
        fputs(str, pid_file);
        fclose(pid_file);
        return 0;
    }
}

static int stop_daemon(const char *pid_file_path, int exist)
{
    if (exist)
    {
        FILE *pid_file = fopen(pid_file_path, "r");
        int pid = get_pid_from_file(pid_file);
        fclose(pid_file);
        /* Check if the process is running */
        if (process_alive(pid))
        {
            kill(pid, SIGINT);
        }
        /* Delete the pid_file */
        if (remove(pid_file_path) == -1)
        {
            fprintf(stderr, "Fail to delete %s\n", pid_file_path);
            return 1;
        }
    }
    return 0;
}

int daemonize(char *action, char *pid_file_path)
{
    int ret = 0;
    int exist = file_exists(pid_file_path);
    if (strcmp(action, "start") == 0)
    {
        ret = start_daemon(pid_file_path, exist);
    }
    else if (strcmp(action, "stop") == 0)
    {
        ret = stop_daemon(pid_file_path, exist);
    }
    else if (strcmp(action, "reload") == 0)
    {
        // TODO: Implement for bonus
    }
    else if (strcmp(action, "restart") == 0)
    {
        stop_daemon(pid_file_path, exist);
        ret = start_daemon(pid_file_path, exist);
    }
    return ret;
}
