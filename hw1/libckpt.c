/* libckpt.c */
#define _GNU_SOURCE
#include <ucontext.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <assert.h>
#include <sys/mman.h>
#include <unistd.h>

#define NAME_LEN            80
#define MAX_CKPT_SEGMENTS   1000
#define CKPT_FILE           "save-restore.dat"
#define BUF_SIZE            512

struct ckpt_segment {
    void *start;            // beginning of /proc/self/maps region
    void *end;              // end of /proc/self/maps region
    char rwxp[4];           // read, write, execute, shared or private 
    char name[NAME_LEN];    // filename of /proc/self/maps region
    int is_reg_context;     // header for ucontext_t data
    size_t data_size;       // size of following data
};

int 
get_proc_maps_line(int proc_maps_fd, struct ckpt_segment *ckpt_segment, char *name)
{
    unsigned long int start, end;
    char rwxp[4];
    int tmp_stdin, rc;
    tmp_stdin = dup(0);
    dup2(proc_maps_fd, 0);
    rc = scanf("%lx-%lx %4c %*s %*s %*[0-9 ]%[^\n]\n",
               &start, &end, rwxp, name);
    clearerr(stdin);
    dup2(tmp_stdin, 0);
    close(tmp_stdin);
    if (rc == 0 || rc == EOF) {
#ifdef DEBUG
    if (rc == 0) 
        puts("rc == 0");
    else
        puts("rc == EOF");
    int tmp_stdin;
    tmp_stdin = dup(0);
    dup2(proc_maps_fd, 0);
    char buf[BUF_SIZE];
    scanf("%s\n", buf);
    buf[BUF_SIZE - 1] = '\0';
    clearerr(stdin);
    dup2(tmp_stdin, 0);
    close(tmp_stdin);
    printf("Current line: %s\n", buf);
#endif
        ckpt_segment->start = NULL;
        ckpt_segment->end = NULL;
        return EOF;
    } else if (rc == 3) {
        strncpy(ckpt_segment->name, "ANONYMOUS_SEGMENT", 
                strlen("ANONYMOUS_SEGMENT") + 1);
    } else {
        assert(rc == 4);
        strncpy(ckpt_segment->name, name, NAME_LEN - 1);
        ckpt_segment->name[NAME_LEN - 1] = '\0';
    }
    memcpy((void *)ckpt_segment->rwxp, (void *)rwxp, 4);
    ckpt_segment->start = (void *)start;
    ckpt_segment->end = (void *)end;
    ckpt_segment->is_reg_context = 0;
    ckpt_segment->data_size = 0;
    return 0;
}

int 
save_proc_self_maps(int ckpt_fd, struct ckpt_segment ckpt_segments[])
{
    char name[NAME_LEN];
    int proc_maps_fd, rc;
    if ((proc_maps_fd = open("/proc/self/maps", O_RDONLY)) == -1) {
        perror("open (/proc/self/maps)");
        return -1;
    }
    rc = 0;
    int i = 0;
    for (; rc != EOF; ++i) {
        rc = get_proc_maps_line(proc_maps_fd, &ckpt_segments[i], name);
        if (rc == 0) {
            ckpt_segments[i].rwxp[4] = '\0';
            char buf[BUF_SIZE];
            if (sprintf(buf, "%p-%p %s %s %d %zu\n",
                        ckpt_segments[i].start, ckpt_segments[i].end,
                        ckpt_segments[i].rwxp, ckpt_segments[i].name,
                        ckpt_segments[i].is_reg_context,
                        ckpt_segments[i].data_size) < 0) {
                perror("sprintf");
                return -1;
            }
            buf[strlen(buf)] = '\0';
            int rc2, tmp;
            rc2 = 0;
            while (rc2 < strlen(buf)) {
                if ((tmp = write(ckpt_fd, buf + rc2, strlen(buf) - rc2)) == -1) {
                    perror("write");
                    return -1;
                }
                rc2 += tmp;
            }
            assert(rc2 == strlen(buf));
        }
    }
    close(proc_maps_fd);
    return i;
}

int
save_context(int ckpt_fd, ucontext_t *ucp, int i,
             struct ckpt_segment ckpt_segments[])
{
    ckpt_segments[i].start = NULL;
    ckpt_segments[i].end = NULL;
    strncpy(ckpt_segments[i].rwxp, "----", 5);
    strncpy(ckpt_segments[i].name, "CONTEXT", strlen("CONTEXT") + 1);
    ckpt_segments[i].is_reg_context = 1;
    ckpt_segments[i].data_size = sizeof(*ucp);
    char buf[BUF_SIZE];
    if (sprintf(buf, "%p-%p %s %s %d %zu\n",
                ckpt_segments[i].start, ckpt_segments[i].end,
                ckpt_segments[i].rwxp, ckpt_segments[i].name,
                ckpt_segments[i].is_reg_context,
                ckpt_segments[i].data_size) < 0) {
        perror("sprintf");
        return -1;
    }
    buf[strlen(buf)] = '\0';
    int rc2, tmp;
    rc2 = 0;
    while (rc2 < strlen(buf)) {
        if ((tmp = write(ckpt_fd, buf + rc2, strlen(buf) - rc2)) == -1) {
            perror("write");
            return -1;
        }
        rc2 += tmp;
    }
    assert(rc2 == strlen(buf));
    rc2 = 0;
    while (rc2 < sizeof(*ucp)) {
        if ((tmp = write(ckpt_fd, (char *)ucp + rc2, sizeof(*ucp) - rc2)) == -1) {
            perror("write");
            return -1;
        }
        rc2 += tmp;
    }
    assert(rc2 == sizeof(*ucp));
    return 0;
}

void 
sig_handler(int signum)
{
#ifdef DEBUG
    printf("Current /proc/self/maps:\n");
    pid_t pid = getpid();
    char command[BUF_SIZE];
    sprintf(command, "cat /proc/%d/maps\n", pid);
    command[BUF_SIZE - 1] = '\0';
    system(command);
#endif
    ucontext_t uc;
    getcontext(&uc);
    struct ckpt_segment ckpt_segments[MAX_CKPT_SEGMENTS];
    int ckpt_fd;
    static int is_restart = 0;
    if (signum == SIGUSR2) {
        if (is_restart) {
            puts("Restarting...");
            is_restart = 0;
        } else {
            puts("Checkpointing...");
            if ((ckpt_fd = open(CKPT_FILE, O_WRONLY | O_CREAT | O_TRUNC,
                                S_IRUSR | S_IWUSR)) == -1) {
                perror("open");
                exit(EXIT_FAILURE);
            }
            int i;
            if ((i = save_proc_self_maps(ckpt_fd, ckpt_segments)) == -1)
                exit(EXIT_FAILURE);
        #ifdef DEBUG
            printf("# of /proc/self/maps regions: %d\n", i - 1);
        #endif
            if (save_context(ckpt_fd, &uc, i, ckpt_segments) == -1)
                exit(EXIT_FAILURE);
            is_restart = 1;
        }
    }
}

void
__attribute__((constructor)) init()
{
    signal(SIGUSR2, (void *)sig_handler);
}
