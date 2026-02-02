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

#define MAXARGS 100
#define BUFSIZE 128

pid_t child_pid;

void
sig_handler(int signum)
{
    if (kill(child_pid, SIGKILL) < 0) {
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
    signal(SIGINT, sig_handler);
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
        if (parse_argv(buf, argv) < 0)
            exit(EXIT_FAILURE);
        child_pid = fork();
        switch (child_pid) {
            case -1:
                perror("fork");
                exit(EXIT_FAILURE);
            case 0:
                if (execvp(argv[0], &argv[0]) < 0) {
                    perror("execvp");
                    exit(EXIT_FAILURE);
                }
            default:
                wait(NULL);
        }
        free_argv(argv);
    }
    exit(EXIT_SUCCESS);
}
