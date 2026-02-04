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
#include <ucontext.h>

#define MAXARGS           100
#define BUFSIZE           100 
#define RD_PIPE           0
#define WR_PIPE           1

ucontext_t uc;

// shell process does not get killed by SIGINT; child shell processes
// exit from SIGINT by setting signal(SIGINT, SIG_DFL) for default behavior
void
sig_handler(int signum)
{
    assert(signum == SIGINT);
    putchar('\n');
    // setcontext to start of main shell loop to avoid shell process
    // returning to a random instruction within the shell loop and not
    // properly reprompting the user
    if (setcontext(&uc) < 0)
        return;
}

// check if command uses a pipe (e.g. ls | wc)
int
has_pipe(char *buf)
{
    char *substr, *l, *r;
    if ((substr = strstr(buf, " | ")) == NULL)
        return 0;
    // if there is a pipe within quotations (e.g. echo 'ls | wc')
    // then this argument should not be interpreted as a pipe
    if ((l = index(buf, '\'')) != NULL)
        if ((r = rindex(buf, '\'')) != NULL)
            if (l < substr && r > substr + 2)
                return 0;
    if ((l = index(buf, '\"')) != NULL)
        if ((r = rindex(buf, '\"')) != NULL)
            if (l < substr && r > substr + 2)
                return 0;
    return 1;
}

// check if command uses output redirection (e.g. ls -l > tmp.txt)
int
has_out_redirect(char *buf)
{
    char *substr, *l, *r;
    if ((substr = strstr(buf, " > ")) == NULL)
        return 0;
    // interpret '>' within quotations as a string literal
    if ((l = index(buf, '\'')) != NULL)
        if ((r = rindex(buf, '\'')) != NULL)
            if (l < substr && r > substr + 2)
                return 0;
    if ((l = index(buf, '\"')) != NULL)
        if ((r = rindex(buf, '\"')) != NULL)
            if (l < substr && r > substr + 2)
                return 0;
    return 1;
}

// check if command uses input redirection (e.g. wc < tmp.txt)
int
has_in_redirect(char *buf)
{
    char *substr, *l, *r;
    if ((substr = strstr(buf, " < ")) == NULL)
        return 0;
    if ((l = index(buf, '\'')) != NULL)
        if ((r = rindex(buf, '\'')) != NULL)
            if (l < substr && r > substr + 2)
                return 0;
    if ((l = index(buf, '\"')) != NULL)
        if ((r = rindex(buf, '\"')) != NULL)
            if (l < substr && r > substr + 2)
                return 0;
    return 1;
}

// checks if the command is to spawn only a background process
// and no other process (e.g. sleep 10 &)
int
has_only_background(char *buf)
{
    char *substr, *l, *r;
    if ((substr = strstr(buf, " &")) == NULL)
        return 0;
    if ((l = index(buf, '\'')) != NULL)
        if ((r = rindex(buf, '\'')) != NULL)
            if (l < substr && r > substr + 2)
                return 0;
    if ((l = index(buf, '\"')) != NULL)
        if ((r = rindex(buf, '\"')) != NULL)
            if (l < substr && r > substr + 2)
                return 0;
    int n = strlen(buf);
    if (substr[2] == '\n' || substr[2] == '\0')
        return 1;
    for (int ix = 2; ix < n; ++ix)
        if (substr[ix] != ' ')
            return 0;
    return 1;
}

// check if the command is to spawn one background process
// and one other process (e.g. sleep 10 & wc tmp.txt)
int
has_one_background(char *buf)
{
    char *substr, *l, *r;
    if ((substr = strstr(buf, " & ")) == NULL)
        return 0;
    if ((l = index(buf, '\'')) != NULL)
        if ((r = rindex(buf, '\'')) != NULL)
            if (l < substr && r > substr + 2)
                return 0;
    if ((l = index(buf, '\"')) != NULL)
        if ((r = rindex(buf, '\"')) != NULL)
            if (l < substr && r > substr + 2)
                return 0;
    int n = strlen(substr);
    if (substr[2] == '\n' || substr[2] == '\0')
        return 0;
    for (int ix = 2; ix < n; ++ix)
        if (substr[ix] != ' ')
            return 1;
    return 0;
}

// strdup/strndup call malloc so args must be freed
void
free_argv(char *argv[])
{
    for (int ix = 0; argv[ix] != NULL; ++ix)
        free(argv[ix]);
}

// for parsing commands that do not involve a pipe, redirection or
// a background task
int 
parse_argv(char *buf, char *argv[])
{
    int ix, argc = 0, prev = 0, n = strlen(buf);
    for (ix = 0; ix < n;) {
        if (buf[ix] == '\n') {
            buf[ix] = '\0';
            break;
        } else if (buf[ix] == '\'' || buf[ix] == '\"') {
            char delim = (buf[ix] == '\'') ? '\'' : '\"';
            while (buf[++ix] != delim)
                ;
            ++prev;
            if ((argv[argc++] = strndup(&buf[prev], ix - prev)) == NULL) {
                perror("strndup");
                goto fail;
            }
            while (buf[++ix] == ' ')
                ;
            prev = ix;
            continue;
        } else if (buf[ix] == ' ') {
            if ((argv[argc++] = strndup(&buf[prev], ix - prev)) == NULL) {
                perror("strndup");
                goto fail;
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
            goto fail;
        }
    }
    argv[argc] = NULL;
    return 0;
    fail:
        argv[argc] = NULL;
        free_argv(argv);
        return -1;
}

// parse command involving a pipe (only handles one pipe/two processes)
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
                goto fail;
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
                goto fail;
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
            goto fail;
        }
    }
    argv2[argc2] = NULL;
    return 0;
    fail:
        argv1[argc1] = NULL;
        argv2[argc2] = NULL;
        free_argv(argv1);
        free_argv(argv2);
        return -1;
}

// parse command involving output or input redirection
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
                free_argv(argv);
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

// parse command involving only a background task (e.g. sleep 10 &)
int
parse_argv_only_background(char *buf, char *argv[])
{
    int prev = 0, argc = 0, n = strlen(buf);
    for (int ix = 0; ix < n;) {
        if (buf[ix] == '&') {
            argv[argc] = NULL;
            break;
        } else if (buf[ix] == ' ') {
            if ((argv[argc++] = strndup(&buf[prev], ix - prev)) == NULL) {
                perror("strndup");
                free_argv(argv);
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

// parse command involving a background task as well as another
// typical process (e.g. sleep 10 & wc tmp.txt)
int
parse_argv_one_background(char *buf, char *argv1[], char *argv2[])
{
    int ix, prev = 0, argc1 = 0, argc2 = 0, n = strlen(buf);
    for (ix = 0; ix < n;) {
        if (buf[ix] == '&') {
            argv1[argc1] = NULL;
            while (buf[++ix] == ' ')
                ;
            prev = ix;
            break;
        } else if (buf[ix] == ' ') {
            if ((argv1[argc1++] = strndup(&buf[prev], ix - prev)) == NULL) {
                perror("strndup");
                goto fail;
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
                goto fail;
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
            goto fail;
        }
    }
    argv2[argc2] = NULL;
    return 0;
    fail:
        argv1[argc1] = NULL;
        argv2[argc2] = NULL;
        free_argv(argv1);
        free_argv(argv2);
        return -1;
}

// spawn a single child process
void
spawn_child(char *argv[])
{
    pid_t childpid = fork();
    switch (childpid) {
        case -1:
            perror(argv[0]);
            break;
        case 0:
            signal(SIGINT, SIG_DFL);
            if (execvp(argv[0], &argv[0]) < 0) {
                perror(argv[0]);
                exit(EXIT_FAILURE);
            }
        default:
            if (waitpid(childpid, NULL, 0) < 0)
                perror(argv[0]);
            break;
    }
    free_argv(argv);
    return;
}

// spawn a child process that redirects output to a file
void
spawn_child_out_redirect(char *argv[], char *filename)
{
    int fd;
    if ((fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, S_IRWXU)) < 0) {
        perror(filename);
        goto end;
    }
    pid_t childpid = fork();
    switch (childpid) {
        case -1:
            perror(argv[0]);
            goto end;
        case 0:
            signal(SIGINT, SIG_DFL);
            if (close(STDOUT_FILENO) < 0) {
                perror(argv[0]);
                exit(EXIT_FAILURE);
            }
            dup2(fd, STDOUT_FILENO);
            if (execvp(argv[0], &argv[0]) < 0) {
                perror(argv[0]);
                exit(EXIT_FAILURE);
            }
        default:
            if (waitpid(childpid, NULL, 0) < 0)
                perror(argv[0]);
            goto end;
    }
    end:
        free_argv(argv); 
        if (fd != -1)
            close(fd);
    return;
}

// spawn a child process that redirects input from a file
void
spawn_child_in_redirect(char *argv[], char *filename)
{
    int fd;
    if ((fd = open(filename, O_RDONLY)) < 0) {
        perror(filename);
        goto end;
    }
    pid_t childpid = fork();
    switch (childpid) {
        case -1:
            perror(argv[0]);
            goto end;
        case 0:
            signal(SIGINT, SIG_DFL);
            if (close(STDIN_FILENO) < 0) {
                perror(argv[0]);
                exit(EXIT_FAILURE);
            }
            dup2(fd, STDIN_FILENO);
            if (execvp(argv[0], &argv[0]) < 0) {
                perror(argv[0]);
                exit(EXIT_FAILURE);
            }
        default:
            if (waitpid(childpid, NULL, 0) < 0)
                perror(argv[0]);
            goto end;
    }
    end:
        free_argv(argv);
        if (fd != -1)
            close(fd);
    return;
}

// spawn a child process to run in the background
void
spawn_child_background(char *argv[], int background_id)
{
    switch (fork()) {
        case -1:
            perror(argv[0]);
            break;
        case 0:
            signal(SIGINT, SIG_DFL);
            // background task prints pid to stdout
            // for ease of sending signals
            printf("[%d] %d\n", background_id, getpid());
            if (execvp(argv[0], &argv[0]) < 0) {
                perror(argv[0]);
                exit(EXIT_FAILURE);
            }
        default:
            break;
    }
    free_argv(argv);
    return;
}

// spawn one child process in the background and another
// blocking child process
void
spawn_one_background(char *argv1[], char *argv2[], int background_id)
{
    // background process
    switch (fork()) {
        case -1:
            perror(argv1[0]);
            break;
        case 0:
            signal(SIGINT, SIG_DFL);
            // background task prints pid to stdout
            printf("[%d] %d\n", background_id, getpid());
            if (execvp(argv1[0], &argv1[0]) < 0) {
                perror(argv1[0]);
                exit(EXIT_FAILURE);
            }
        default:
            break;
    }
    // blocking child process
    pid_t childpid;
    switch (childpid) {
        case -1:
            perror(argv2[0]);
            break;
        case 0:
            signal(SIGINT, SIG_DFL);
            if (execvp(argv2[0], &argv2[0]) < 0) {
                perror(argv2[0]);
                exit(EXIT_FAILURE);
            }
        default:
            if (waitpid(childpid, NULL, 0) < 0)
                perror(argv2[0]);
            break;
    }
    free_argv(argv1);
    free_argv(argv2);
    return;
}

// spawn two child processes that communicate using a pipe
void
spawn_children_pipe(char *argv1[], char *argv2[])
{
    int pipefds[2];
    int fd, pipe_rc;
    if ((pipe_rc = pipe(pipefds)) < 0) {
        char buf[128];
        snprintf(buf, 128, "%s, %s", argv1[0], argv2[0]);
        perror(buf);
        goto end;
    }
    pid_t childpid1 = fork();
    switch (childpid1) {
        case -1:
            perror(argv1[0]);
            goto end;
        case 0:
            signal(SIGINT, SIG_DFL);
            close(STDOUT_FILENO);
            if ((fd = dup(pipefds[WR_PIPE])) < 0) {
                perror(argv1[0]);
                exit(EXIT_FAILURE);
            }
            if (close(pipefds[RD_PIPE]) < 0) {
                perror(argv1[0]);
                exit(EXIT_FAILURE);
            }
            if (close(pipefds[WR_PIPE]) < 0) {
                perror(argv1[0]);
                exit(EXIT_FAILURE);
            }
            if (execvp(argv1[0], &argv1[0]) < 0) {
                perror(argv1[0]);
                exit(EXIT_FAILURE);
            }
        default:
            break;
    }
    pid_t childpid2 = fork();
    switch (childpid2) {
        case -1:
            perror(argv2[0]);
            goto end;
        case 0:
            signal(SIGINT, SIG_DFL);
            close(STDIN_FILENO);
            if ((fd = dup(pipefds[RD_PIPE])) < 0) {
                perror(argv2[0]);
                exit(EXIT_FAILURE);
            }
            if (close(pipefds[RD_PIPE]) < 0) {
                perror(argv2[0]);
                exit(EXIT_FAILURE);
            }
            if (close(pipefds[WR_PIPE]) < 0) {
                perror(argv2[0]);
                exit(EXIT_FAILURE);
            }
            if (execvp(argv2[0], &argv2[0]) < 0) {
                perror(argv2[0]);
                exit(EXIT_FAILURE);
            }
        default:
            goto end;
    }
    end:
        if (pipe_rc != -1) {
            close(pipefds[RD_PIPE]); 
            close(pipefds[WR_PIPE]);
        }
        if (childpid1 != -1)
            if (waitpid(childpid1, NULL, 0) < 0)
                perror(argv1[0]);
        if (childpid2 != -1)
            if (waitpid(childpid2, NULL, 0) < 0)
                perror(argv2[0]);
        free_argv(argv1);
        free_argv(argv2);
    return;
}

// main prompt loop that delegates command parsing and spawning child processes
// to appropiate helper functions 
void
shell_loop()
{
    static int background_id = 1;
    while (1) {
        getcontext(&uc);
        char buf[BUFSIZE];
        printf("$ ");
        fflush(stdout);
        if (fgets(buf, BUFSIZE, stdin) == NULL && feof(stdin))
            return;
        if (has_pipe(buf)) {
            char *argv1[MAXARGS];
            char *argv2[MAXARGS];
            if (parse_argv_pipe(buf, argv1, argv2) < 0)
                continue;
            spawn_children_pipe(argv1, argv2);
        } else if (has_out_redirect(buf)) {
            char *argv[MAXARGS];
            char filename[BUFSIZE];
            if (parse_argv_redirect(buf, argv, filename) < 0)
                continue;
            spawn_child_out_redirect(argv, filename);
        } else if (has_in_redirect(buf)) {
            char *argv[MAXARGS];
            char filename[BUFSIZE];
            if (parse_argv_redirect(buf, argv, filename) < 0)
                continue;
            spawn_child_in_redirect(argv, filename);
        } else if (has_only_background(buf)) {
            char *argv[MAXARGS];
            if (parse_argv_only_background(buf, argv) < 0)
                continue;
            spawn_child_background(argv, background_id++);
        } else if (has_one_background(buf)) {
            char *argv1[MAXARGS];
            char *argv2[MAXARGS];
            if (parse_argv_one_background(buf, argv1, argv2) < 0)
                continue;
            spawn_one_background(argv1, argv2, background_id++);
        } else {
            char *argv[MAXARGS];
            if (parse_argv(buf, argv) < 0)
                continue;
            spawn_child(argv);
        }
    }
    return;
}

int
main(int argc, char *argv[])
{
    // register signal handler for SIGINT
    signal(SIGINT, sig_handler);
    shell_loop();
    exit(0);
}
