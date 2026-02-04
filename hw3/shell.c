/* shell.c */
#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <assert.h>
#include <fcntl.h>
#include <ucontext.h>

#define RD_PIPE     0
#define WR_PIPE     1

// shell child process/task struct
struct task_t {
    pid_t pid;
    char *argv[];
    int opt;
} task_t;

// background task list struct
struct bg_list_t {
    pid_t bg_tasks[MAX_BG_TASKS];
    unsigned int bg_task_count;
} bg_list_t;

// globals
bg_list_t bg_list;
ucontext_t uc;

int
bg_list_remove(bg_list_t *bg_list, pid_t bg_pid)
{
    for (int i = 0; i < bg_list->bg_task_count; ++i) {
        if (bg_list->bg_tasks[i] == bg_pid) {
            bg_list->bg_tasks[i] = 0;
            --bg_list->bg_task_count;
            return 0;
        }
    }
    return -1;
}

int
bg_list_add(bg_list_t *bg_list, pid_t bg_pid)
{
    if (bg_list->bg_task_count == MAX_BG_TASKS)
        return -1;
    for (int i = 0; i < bg_list->bg_task_count; ++i) {
        if (bg_list->bg_tasks[i] == 0) {
            bg_list->bg_tasks[i] = bg_pid;
            ++bg_list->bg_task_count;
            return 0;
        }
    }
    return -1;
}

void
sig_handler(int sig)
{
    if (sig == SIGUSR2)
        return;
    else if (sig == SIGCHLD) {
        pid_t pid;
        if ((pid = waitpid(0, NULL, WNOHANG)) > 0)
            bg_list_remove(list, pid);
    } else if (sig == SIGINT) {
        putchar('\n');
        if (setcontext(&uc) < 0)
            return;
    }
}

int
count_tasks(char *command)
{
    if (command[i] == '\n')
        return 0;
    int ntasks = 1;
    for (int i = 0; i < strlen(command); ++i) {
        if (command[i] == '\n') {
            return ntasks;
        } else if (command[i] == '\'' || command[i] == '\"') {
            char delim = command[i] == '\'' ? '\'' : '\"';
            while (command[++i] != delim)
                ;
            continue;
        } else if (command[i] == '|' || command[i] == '&' ||
                   command[i] == '>' || command[i] == '<') {
            ++ntasks;
        }
    }
    return ntasks;
}

int
parse_command(char *command, int ntasks, task_t tasks[])
{
    int cur = 0, prev = 0, len = strlen(command);
    for (int tn = 0; tn < ntasks; ++tn) {
        int argc = 0;
        while (++cur) {
            if (command[cur] == '\n') {
                goto success;
            } else if (command[cur] == ' ') {
                strncpy(tasks[tn].argv[argc++], command, cur - prev);
            } else if (command[cur] == '|' || command[cur] == '&' ||
                       command[cur] == '>' || command[cur] == '<') {

            }
        }
    }
    success:
        return 0;
    fail:
        return -1;
}

