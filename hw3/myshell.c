/* myshell.c */
#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <sys/types.h>
#include <fcntl.h>
#include <assert.h>

#define MAXARGS 100
#define BUFSIZE 128

static pid_t childpid = 0;

void
sighandler(int signum)
{
    if (!childpid || getpid() == childpid)
        return;
    printf("Sending SIGKILL to %d\n", childpid);
    if (kill(childpid, SIGKILL) < 0) {
        perror("kill");
        exit(EXIT_FAILURE);
    }
    return;
}

void
free_argv(char *argv[])
{
    for (int ix = 0; argv[ix] != NULL; ++ix)
        free(argv[ix]);
}

int
has_pipe(char *buf) 
{
    int n = strlen(buf);
    for (int ix = 0; ix <= n - 3; ++ix)
        if (!strncmp(&buf[ix], " | ", 3))
            return 1;
    return 0;
}

int
parse_argv(char *buf, char *argv[])
{
    int argc = 0, previx = 0, n = strlen(buf);
    for (int ix = 0; ix < n; ++ix) {
        if (buf[ix] == '\n') {
            buf[ix] = '\0';
            break;
        } else if (buf[ix] == ' ') {
            argv[argc] = strndup(&buf[previx], ix - previx);
            if (argv[argc++] == NULL) {
                perror("strndup");
                return -1;
            }
            previx = ix + 1;
        }
    }
    argv[argc++] = strndup(&buf[previx], n - previx);
    argv[argc] = NULL;
    return 0;
}

int
main()
{
    signal(SIGINT, sighandler);
    while (1) {
        char buf[BUFSIZE];
        char *argv[MAXARGS];
        printf("$ ");
        if (fflush(stdout) == EOF) {
            perror("fflush");
            exit(EXIT_FAILURE);
        }
        if (fgets(buf, BUFSIZE, stdin) == NULL && feof(stdin)) {
            fprintf(stderr, "Failed to read from stdin: %s\n",
                    strerror(errno));
            exit(EXIT_FAILURE);
        }
        if (has_pipe(buf)) {
            puts("commands with pipes can not be handled");
            continue;
        }
        if (parse_argv(buf, argv) < 0)
            exit(EXIT_FAILURE);
        childpid = fork();
        int wstatus;
        switch (childpid) {
            case -1:
                perror("fork");
                exit(EXIT_FAILURE);
            case 0:
                if (execvp(argv[0], &argv[0]) < 0) {
                    perror("execvp");
                    exit(EXIT_FAILURE);
                }
            default:
                if (waitpid(childpid, &wstatus, 0) < 0) {
                    perror("waitpid");
                    exit(EXIT_FAILURE);
                }
                if (WIFSIGNALED(wstatus))
                    if (WTERMSIG(wstatus) == SIGKILL)
                        printf("%d terminated by SIGKILL\n",
                                childpid);
        }
        free_argv(argv);
    }
    exit(EXIT_SUCCESS);
}
