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

#define MAXARGS           100
#define BUFSIZE           100 
#define RD_PIPE           0
#define WR_PIPE           1

static pid_t pids[2] = { 0, 0 };

void
sig_handler(int signum)
{
    assert(signum == SIGINT);
    if (pids[0] == getpid() || pids[1] == getpid())
        exit(EXIT_SUCCESS);
    printf("\n$ ");
    fflush(stdout);
    return;
}

// e.g. ls | wc
int
has_pipe(char *buf)
{
    if (strstr(buf, " | ") == NULL)
        return 0;
    return 1;
}

// e.g. ls -l > tmp.txt
int
has_out_redirect(char *buf)
{
    if (strstr(buf, " > ") == NULL)
        return 0;
    return 1;
}

// e.g. wc < tmp.txt
int
has_in_redirect(char *buf)
{
    if (strstr(buf, " < ") == NULL)
        return 0;
    return 1;
}

// e.g. sleep 10 &
int
has_background(char *buf)
{
    if (strstr(buf, " &") == NULL)
        return 0;
    return 1;
}

#ifdef DEBUG
void
print_argv(char *argv[])
{
    printf("argv: ");
    for (int ix = 0; argv[ix] != NULL; ++ix)
        printf("%s ", argv[ix]);
    printf("\n");
}
#endif

// strdup/strndup call malloc so args must be freed
void
free_argv(char *argv[])
{
    for (int ix = 0; argv[ix] != NULL; ++ix)
        free(argv[ix]);
}

int 
parse_argv(char *buf, char *argv[])
{
    int ix, argc = 0, prev = 0, n = strlen(buf);
    for (ix = 0; ix < n;) {
        if (buf[ix] == '\n') {
            buf[ix] = '\0';
            break;
        } else if (buf[ix] == ' ') {
            if ((argv[argc++] = strndup(&buf[prev], ix - prev)) == NULL) {
                perror("strndup");
                return -1;
            }
            while (buf[++ix] == ' ')
                ;
            prev = ix;
            continue;
        }
        ++ix;
    }
    if (prev != ix) {
        if ((argv[argc++] = strndup(&buf[prev], n - prev)) == NULL) {
            perror("strndup");
            return -1;
        }
    }
    argv[argc] = NULL;
    return 0;
}

int
parse_argv_pipe(char *buf, char *argv1[], char *argv2[])
{
    int ix, prev, n, argc1, argc2;
    prev = argc1 = argc2 = 0;
    n = strlen(buf);
    for (ix = 0; ix < n;) {
        if (buf[ix] == '|') {
            argv1[argc1] = NULL;
            while (buf[++ix] == ' ')
                ;
            prev = ix;
            break;
        } else if (buf[ix] == ' ') {
            if ((argv1[argc1++] = strndup(&buf[prev], ix - prev)) == NULL) {
                perror("strndup");
                return -1;
            }
            while (buf[++ix] == ' ')
                ;
            prev = ix;
            continue;
        }
        ++ix;
    }
    for (; ix < n;) {
        if (buf[ix] == '\n') {
            buf[ix] = '\0';
            break;
        } else if (buf[ix] == ' ') {
            if ((argv2[argc2++] = strndup(&buf[prev], ix - prev)) == NULL) {
                perror("strndup");
                return -1;
            }
            while (buf[++ix] == ' ')
                ;
            prev = ix;
            continue;
        }
        ++ix;
    }
    if (prev != ix) {
        if ((argv2[argc2++] = strndup(&buf[prev], n - prev)) == NULL) {
            perror("strndup");
            return -1;
        }
    }
    argv2[argc2] = NULL;
    return 0;
}

int
parse_argv_redirect(char *buf, char *argv[], char *filename)
{
    int ix, prev = 0, argc = 0, n = strlen(buf);
    for (ix = 0; ix < n;) {
        if (buf[ix] == '>' || buf[ix] == '<') {
            argv[argc] = NULL;
            while (buf[++ix] == ' ')
                ;
            prev = ix;
            break;
        } else if (buf[ix] == ' ') {
            if ((argv[argc++] = strndup(&buf[prev], ix - prev)) == NULL) {
                perror("strndup");
                return -1;
            }
            while (buf[++ix] == ' ')
                ;
            prev = ix;
            continue;
        }
        ++ix;
    }
    if (buf[n - 1] = '\n' && buf[n - 2] != ' ')
        buf[n - 1] = '\0';
    else {
        assert(buf[n - 2] == ' ');
        n -= 2;
        while (buf[n--] == ' ')
            ;
        buf[++n] = '\0';
    }
    strncpy(filename, &buf[prev], n - prev);
    return 0;
}

int
parse_argv_background(char *buf, char *argv[])
{
    int prev = 0, argc = 0, n = strlen(buf);
    for (int ix = 0; ix < n;) {
        if (buf[ix] == '&') {
            argv[argc] = NULL;
            break;
        } else if (buf[ix] == ' ') {
            if ((argv[argc++] = strndup(&buf[prev], ix - prev)) == NULL) {
                perror("strndup");
                return -1;
            }
            while (buf[++ix] == ' ')
                ;
            prev = ix;
            continue;
        }
        ++ix;
    }
    return 0;
}

int
spawn_child(char *argv[])
{
    pids[0] = fork();
    switch (pids[0]) {
        case -1:
            perror("fork");
            goto fail;
        case 0:
            if (execvp(argv[0], &argv[0]) < 0) {
                fprintf(stderr, "execvp (%s): %s\n", argv[0], strerror(errno));
                exit(EXIT_FAILURE);
            }
        default:
            if (waitpid(pids[0], NULL, 0) < 0) {
                perror("waitpid");
                goto fail;
            }
            goto success;
    }
    success:
        pids[0] = 0;
        free_argv(argv);
        return 0;
    fail:
        pids[0] = 0;
        free_argv(argv);
        return -1;
}

int
spawn_child_out_redirect(char *argv[], char *filename)
{
#ifdef DEBUG
    print_argv(argv);
    printf("filename: %s\n", filename);
#endif
    int fd;
    if ((fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, S_IRWXU)) < 0) {
        perror("open");
        goto fail;
    }
    pids[0] = fork();
    switch (pids[0]) {
        case -1:
            perror("fork");
            goto fail;
        case 0:
            if (close(STDOUT_FILENO) < 0) {
                perror("close");
                exit(EXIT_FAILURE);
            }
            dup2(fd, STDOUT_FILENO);
            if (execvp(argv[0], &argv[0]) < 0) {
                fprintf(stderr, "execvp (%s): %s\n", argv[0], strerror(errno));
                exit(EXIT_FAILURE);
            }
        default:
            if (waitpid(pids[0], NULL, 0) < 0) {
                perror("waitpid");
                goto fail;
            }
            goto success;
    }
    success:
        pids[0] = 0;
        free_argv(argv);
        close(fd);
        return 0;
    fail:
        pids[0] = 0;
        free_argv(argv);
        if (fd != -1)
            close(fd);
        return -1;
}

int
spawn_child_in_redirect(char *argv[], char *filename)
{
    int fd;
    if ((fd = open(filename, O_RDONLY)) < 0) {
        perror("open");
        goto fail;
    }
    pids[0] = fork();
    switch (pids[0]) {
        case -1:
            perror("fork");
            goto fail;
        case 0:
            if (close(STDIN_FILENO) < 0) {
                perror("close");
                exit(EXIT_FAILURE);
            }
            dup2(fd, STDIN_FILENO);
            if (execvp(argv[0], &argv[0]) < 0) {
                fprintf(stderr, "execvp (%s): %s\n", argv[0], strerror(errno));
                exit(EXIT_FAILURE);
            }
        default:
            if (waitpid(pids[0], NULL, 0) < 0) {
                perror("waitpid");
                goto fail;
            }
            goto success;
    }
    success:
        pids[0] = 0;
        free_argv(argv);
        close(fd);
        return 0;
    fail:
        pids[0] = 0;
        free_argv(argv);
        if (fd != -1)
            close(fd);
        return -1;
}

int
spawn_child_background(char *argv[])
{
    switch (fork()) {
        case -1:
            perror("fork");
            goto fail;
        case 0:
            if (execvp(argv[0], &argv[0]) < 0) {
                fprintf(stderr, "execvp (%s): %s\n", argv[0], strerror(errno));
                exit(EXIT_FAILURE);
            }
        default:
            goto success;
    }
    success:
        free_argv(argv);
        return 0;
    fail:
        free_argv(argv);
        return -1;
}

int
spawn_children_pipe(char *argv1[], char *argv2[])
{
#ifdef DEBUG
    print_argv(argv1);
    print_argv(argv2);
#endif
    int pipefds[2];
    int fd;
    if (pipe(pipefds) < 0) {
        perror("pipe");
        goto fail;
    }
    pids[0] = fork();
    switch (pids[0]) {
        case -1:
            perror("fork (child 1)");
            goto fail;
        case 0:
            if (close(STDOUT_FILENO) < 0) {
                perror("close");
                exit(EXIT_FAILURE);
            }
            if ((fd = dup(pipefds[WR_PIPE])) < 0) {
                perror("dup");
                exit(EXIT_FAILURE);
            }
            if (close(pipefds[RD_PIPE]) < 0) {
                perror("close");
                exit(EXIT_FAILURE);
            }
            if (close(pipefds[WR_PIPE]) < 0) {
                perror("close");
                exit(EXIT_FAILURE);
            }
            if (execvp(argv1[0], &argv1[0]) < 0) {
                fprintf(stderr, "(child 1) execvp (%s): %s\n",
                        argv1[0], strerror(errno));
                exit(EXIT_FAILURE);
            }
        default:
            break;
    }
    pids[1] = fork();
    switch (pids[1]) {
        case -1:
            perror("fork (child 2)");
            goto fail;
        case 0:
            close(STDIN_FILENO);
            if ((fd = dup(pipefds[RD_PIPE])) < 0) {
                perror("dup");
                exit(EXIT_FAILURE);
            }
            if (close(pipefds[RD_PIPE]) < 0) {
                perror("close");
                exit(EXIT_FAILURE);
            }
            if (close(pipefds[WR_PIPE]) < 0) {
                perror("close");
                exit(EXIT_FAILURE);
            }
            if (execvp(argv2[0], &argv2[0]) < 0) {
                fprintf(stderr, "(child 2) execvp (%s): %s\n",
                        argv2[0], strerror(errno));
                exit(EXIT_FAILURE);
            }
        default:
            break;
    }
    if (close(pipefds[RD_PIPE]) < 0) {
        perror("close");
        goto fail;
    }
    if (close(pipefds[WR_PIPE]) < 0) {
        perror("close");
        goto fail;
    }
    if (waitpid(pids[0], NULL, 0) < 0) {
        perror("waitpid (child 1)");
        goto fail;
    }
    if (waitpid(pids[1], NULL, 0) < 0) {
        perror("waitpid (child 2)");
        goto fail;
    }
    goto success;
    fail:
        free_argv(argv1);
        free_argv(argv2);
        pids[0] = 0;
        pids[1] = 0;
        return -1;
    success:
        free_argv(argv1);
        free_argv(argv2);
        pids[0] = 0;
        pids[1] = 0;
        return 0;
}

int
main_loop()
{
    while (1) {
        char buf[BUFSIZE];
        printf("$ ");
        fflush(stdout);
        if (fgets(buf, BUFSIZE, stdin) == NULL && feof(stdin)) {
            fprintf(stderr, "Failed to read from stdin: %s\n", strerror(errno));
            goto fail;
        }
        if (has_pipe(buf)) {
            char *argv1[MAXARGS];
            char *argv2[MAXARGS];
            if (parse_argv_pipe(buf, argv1, argv2) < 0)
                goto fail;
            if (spawn_children_pipe(argv1, argv2) < 0)
                goto fail;
        } else if (has_out_redirect(buf)) {
            char *argv[MAXARGS];
            char filename[BUFSIZE];
            if (parse_argv_redirect(buf, argv, filename) < 0)
                goto fail;
            if (spawn_child_out_redirect(argv, filename) < 0)
                goto fail;
        } else if (has_in_redirect(buf)) {
            char *argv[MAXARGS];
            char filename[BUFSIZE];
            if (parse_argv_redirect(buf, argv, filename) < 0)
                goto fail;
            if (spawn_child_in_redirect(argv, filename) < 0)
                goto fail;
        } else if (has_background(buf)) {
            char *argv[MAXARGS];
            if (parse_argv_background(buf, argv) < 0)
                goto fail;
            if (spawn_child_background(argv) < 0)
                goto fail;
        } else {
            char *argv[MAXARGS];
            if (parse_argv(buf, argv) < 0)
                goto fail;
            if (spawn_child(argv) < 0)
                goto fail;
        }
    }
    fail:
        return -1;
    return 0;
}

int
main()
{
    signal(SIGINT, sig_handler);
    if (main_loop() < 0) 
        exit(EXIT_FAILURE);
    exit(EXIT_SUCCESS);
}
