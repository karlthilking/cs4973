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
#include <stdint.h>

#define BUFSIZE         128
#define FILENAME_LEN    64
#define MAXARGS         32
#define RD_PIPE         0
#define WR_PIPE         1
#define PIPE_READER     0b00000001 
#define PIPE_WRITER     0b00000010 
#define BACKGROUND_JOB  0b00000100 
#define REDIRECT_OUT    0b00001000 
#define REDIRECT_IN     0b00010000 
#define READER_MASK     0B00010001 // pipe reader or redirected input
#define WRITER_MASK     0b00001010 // pipe writer or redirected output
#define MAX_BG_TASKS    32

// shell child process/task struct
typedef struct task_t {
    pid_t pid;
    char *argv[MAXARGS];
    uint8_t opt;
    char filename[FILENAME_LEN];
} task_t;

// background task list struct
typedef struct bg_list_t {
    pid_t bg_tasks[MAX_BG_TASKS];
    uint8_t bg_task_count;
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
    pid_t pid;
    switch (sig) {
        case SIGUSR2:
            return;
        case SIGCHLD:
            if ((pid = waitpid(0, NULL, WNOHANG)) > 0)
                if (bg_list_remove(&bg_list, pid) != -1)
                    printf("[%d] %d Done\n", bg_list.bg_task_count + 1, pid);
            return;
        case SIGINT:
            putchar('\n');
            if (setcontext(&uc) < 0)
                perror("setcontext");
            return;
    }
}

int
count_tasks(char *command)
{
    if (command[0] == '\n')
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
        } else if (command[i] == '|') {
            ++ntasks;
        } else if (command[i] == '&') {
            while (command[++i] == ' ')
                ;
            if (command[i] == '\n')
                return ntasks;
            ++ntasks;
        }
    }
    return ntasks;
}

int
parse_command(char *cmd, task_t tasks[], int ntasks)
{
    int cur = 0, prev = 0, len = strlen(cmd);
    for (int tn = 0; tn < ntasks; ++tn) {
        int argc = 0;
        while (1) {
            if (cmd[cur] == '\n') {
                if (prev < cur)
                    strncpy(tasks[tn].argv[argc++], &cmd[prev], cur - prev);
                tasks[tn].argv[argc] = NULL;
                goto success;
            } else if (cmd[cur] == '\'' || cmd[cur] == '\"') {
                char delim = (cmd[cur] == '\'') ? '\'' : '\"';
                while (cmd[++cur] != delim)
                    ;
                ++prev;
                strncpy(tasks[tn].argv[argc++], &cmd[prev], cur - prev);
                while (cmd[++cur] == ' ')
                    ;
                prev = cur;
                continue;
            } else if (cmd[cur] == ' ') {
                strncpy(tasks[tn].argv[argc++], &cmd[prev], cur - prev);
                while (cmd[++cur] == ' ')
                    ;
                prev = cur;
                continue;
            } else if (cmd[cur] == '|' || cmd[cur] == '&' ||
                       cmd[cur] == '>' || cmd[cur] == '<') {
                if (prev < cur)
                    strncpy(tasks[tn].argv[argc++], &cmd[prev], cur - prev);
                switch (cmd[cur]) {
                    case '|':
                        tasks[tn].opt |= PIPE_WRITER;
                        tasks[tn + 1].opt |= PIPE_READER;
                        break;
                    case '&':
                        tasks[tn].opt |= BACKGROUND_JOB;
                        for (int tmp = tn - 1; tmp >= 0; --tmp) {
                            if (tasks[tmp].opt & BACKGROUND_JOB)
                                break;
                            tasks[tmp].opt |= BACKGROUND_JOB;
                        }
                        break;
                    default:
                        if (cmd[cur] == '>')
                            tasks[tn].opt |= REDIRECT_OUT;
                        else
                            tasks[tn].opt |= REDIRECT_IN;
                        while (cmd[++cur] == ' ')
                            ;
                        prev = cur;
                        while (!(cmd[cur] == ' ' || cmd[cur] == '\n' ||
                                 cmd[cur] == '|' || cmd[cur] == '&' ||
                                 cmd[cur] == '<' || cmd[cur] == '>')) {
                            ++cur;
                        }
                        strncpy(tasks[tn].filename, &cmd[prev], cur - prev);
                        break;
                }
                while (cmd[++cur] == ' ')
                    ;
                prev = cur;
                tasks[tn].argv[argc] = NULL;
                break;
            }
            ++cur;
        }
    }
    success:
        return 0;
    fail:
        return -1;
}

void
spawn_tasks(task_t tasks[], int ntasks)
{
    int pending = ntasks;
    int rc, fd, pipefds[2];
    while (pending--) {
        if (tasks[pending].opt & PIPE_READER) {
            if (pipe(pipefds) < 0) {
               fprintf(stderr, "pipe: %s %s\n", 
               tasks[pending].argv[0], tasks[pending - 1].argv[0]);
               continue;
            }
            assert(tasks[pending - 1].opt & PIPE_WRITER);
        }
        tasks[pending].pid = fork();
        switch (tasks[pending].pid) {
            case -1:
                perror(tasks[pending].argv[0]);
                break;
            case 0:
                rc = ~-1;
                fd = ~-1;
                signal(SIGINT, SIG_DFL);
                if (tasks[pending].opt & PIPE_READER) {
                    rc = close(STDIN_FILENO);
                    fd = dup(pipefds[RD_PIPE]);
                } else if (tasks[pending].opt & REDIRECT_IN) {
                    rc = close(STDIN_FILENO);
                    fd = open(tasks[pending].filename, O_RDONLY);
                    dup2(fd, STDIN_FILENO);
                } else if (tasks[pending].opt & PIPE_WRITER) {
                    rc = close(STDOUT_FILENO);
                    fd = dup(pipefds[WR_PIPE]);
                } else if (tasks[pending].opt & REDIRECT_OUT) {
                    rc = close(STDOUT_FILENO);
                    fd = open(tasks[pending].filename, O_WRONLY | O_CREAT |
                              O_TRUNC, S_IRWXU);
                    dup2(fd, STDOUT_FILENO);
                }
                if (fd < 0 || rc < 0) {
                    perror(tasks[pending].argv[0]);
                    exit(EXIT_FAILURE);
                }
                if (tasks[pending].opt & BACKGROUND_JOB) {
                    printf("[%d] %d\n", bg_list.bg_task_count, getpid());
                    kill(getppid(), SIGUSR2);
                }
                close(pipefds[RD_PIPE]);
                close(pipefds[WR_PIPE]);
                if (execvp(tasks[pending].argv[0], tasks[pending].argv) < 0) {
                    perror(tasks[pending].argv[0]);
                    exit(EXIT_FAILURE);
                }
            default:
                if (tasks[pending].opt & BACKGROUND_JOB) {
                    bg_list_add(&bg_list, tasks[pending].pid);
                    pause();
                }
                break;
        }
    }
    for (int i = 0; i < ntasks; ++i) {
        if (tasks[i].opt & BACKGROUND_JOB)
            continue;
        if (waitpid(tasks[i].pid, NULL, 0) < 0)
            perror(tasks[i].argv[0]);
    }
    return;
}

void run_shell()
{
    while (1) {
        getcontext(&uc);
        char buf[BUFSIZE];
        printf("$ ");
        fflush(stdout);
        if (fgets(buf, BUFSIZE, stdin) == NULL && feof(stdin)) {
            putchar('\n');
            goto end;
        }
        int ntasks;
        if ((ntasks = count_tasks(buf)) == 0)
            continue;
        task_t tasks[ntasks];
        if (parse_command(buf, tasks, ntasks) < 0)
            continue;
        spawn_tasks(tasks, ntasks);
    }
    end:
        return;
}

int
main(int argc, char *argv[])
{
    memset((void *)&bg_list, 0, sizeof(bg_list));
    signal(SIGINT, sig_handler);
    signal(SIGUSR2, sig_handler);
    signal(SIGCHLD, sig_handler);
    run_shell();
    exit(0);
}
