/* libckpt.c */
#define _GNU_SOURCE
#include <ucontext.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <assert.h>
#include <unistd.h>

#define NAME_LEN            128
#define MAX_CKPT_SEGMENTS   1000
#define CKPT_FILE           "ckpt.dat"
#define BUF_SIZE            1024

static char *regs[] = {
    "R8", "R9", "R10", "R11", "R12", "R13", "R14", "R15", "RDI",
    "RSI", "RBP", "RBX", "RDX", "RAX", "RCX", "RSP", "RIP",
    "EFL", "CSGSFS", "ERR", "TRAPNO", "OLDMASK", "CR2"
};

struct ckpt_segment {
    void *start;
    void *end;
    char rwxp[4];
    char name[NAME_LEN];
    int is_reg_context;
    size_t data_size;
};

int 
proc_self_maps_line(int proc_maps_fd, struct ckpt_segment *ckpt_segment)
{
    unsigned long int start, end;
    int tmp_stdin, rc;
    tmp_stdin = dup(0);
    dup2(proc_maps_fd, 0);
    rc = scanf("%lx-%lx %4c %*s %*s %*[0-9 ]%[^\n]\n",
               &start, &end, ckpt_segment->rwxp,
               ckpt_segment->name);
    clearerr(stdin);
    dup2(tmp_stdin, 0);
    close(tmp_stdin);
    if (rc == 0 || rc == EOF) {
        ckpt_segment->start = NULL;
        ckpt_segment->end = NULL;
#ifdef DEBUG
    printf("rc == EOF: %p-%p %c%c%c%c %s\n", 
            ckpt_segment->start, ckpt_segment->end,
            ckpt_segment->rwxp[0], ckpt_segment->rwxp[1],
            ckpt_segment->rwxp[2], ckpt_segment->rwxp[3],
            ckpt_segment->name);
#endif
        return EOF;
    } else if (rc == 3) {
        strncpy(ckpt_segment->name, "ANONYMOUS_SEGMENT",
                strlen("ANONYMOUS_SEGMENT") + 1);
        ckpt_segment->name[strlen("ANONYMOUS_SEGMENT")] = '\0';
    } else {
        assert(rc == 4);
        ckpt_segment->name[NAME_LEN - 1] = '\0';
    }
    ckpt_segment->start = (void *)start;
    ckpt_segment->end = (void *)end;
    ckpt_segment->is_reg_context = 0;
    ckpt_segment->data_size = sizeof(*ckpt_segment);
#ifdef DEBUG
    printf("/proc/self/maps line: %p-%p %c%c%c%c %s %d %zu\n", 
           ckpt_segment->start, ckpt_segment->end, 
           ckpt_segment->rwxp[0], ckpt_segment->rwxp[1],
           ckpt_segment->rwxp[2], ckpt_segment->rwxp[3],
           ckpt_segment->name, ckpt_segment->is_reg_context,
           ckpt_segment->data_size);
#endif
    return 0;
}

int 
proc_self_maps(struct ckpt_segment ckpt_segments[])
{
    int i, rc, proc_maps_fd;
    if ((proc_maps_fd = open("/proc/self/maps", O_RDONLY, S_IRUSR)) == -1) {
        perror("open");
        return -1;
    }
    i = 0;
    while (rc != EOF) 
        rc = proc_self_maps_line(proc_maps_fd, &ckpt_segments[i++]);
#ifdef DEBUG
    printf("# /proc/self/maps lines: %d\n", i);
#endif
    return i;
}

int 
save_context(struct ckpt_segment *ckpt_segment) 
{
    ckpt_segment->start = NULL;
    ckpt_segment->end = NULL;
    strncpy(ckpt_segment->rwxp, "----", 4);
    strncpy(ckpt_segment->name, "CONTEXT", strlen("CONTEXT") + 1);
    ckpt_segment->name[strlen("CONTEXT")] = '\0';
    ckpt_segment->is_reg_context = 1;
    ckpt_segment->data_size = sizeof(ucontext_t);
    return 0;
}

int write_ckpt(int ckpt_fd, int n, struct ckpt_segment ckpt_segments[],
               ucontext_t *ucp)
{
    int i, rc, tmp;
    for (i = 0; i < n; ++i) {
        // Write /proc/self/maps segment data
        rc = 0;
        while (rc < ckpt_segments[i].data_size) {
            if ((tmp = write(ckpt_fd, (char *)&ckpt_segments[i] + rc, 
                             ckpt_segments[i].data_size - rc)) == -1) {
                perror("write");
                return -1;
            }
            rc += tmp;
        }
        assert(rc == ckpt_segments[i].data_size);
    }
    rc = 0;
    assert(ckpt_segments[n - 1].data_size == sizeof(*ucp));
    while (rc < sizeof(*ucp)) {
        if ((tmp = write(ckpt_fd, (char *)ucp + rc,
                         sizeof(*ucp) - rc)) == -1) {
            perror("write");
            return -1;
        }
        rc += tmp;
    }
    assert(rc == sizeof(*ucp));
    return 0;
}

void
print_ucontext_regs(ucontext_t *ucp)
{
    int i;
    for (i = 0; i < NGREG; ++i)
        printf("%s: %p\n", regs[i], ucp->uc_mcontext.gregs[i]);
}

void 
sig_handler(int signum)
{
    ucontext_t uc;
    getcontext(&uc);
#ifdef DEBUG
    print_ucontext_regs(&uc);
#endif
    static int is_restart = 0;
    struct ckpt_segment ckpt_segments[MAX_CKPT_SEGMENTS];
    int ckpt_fd;
    if (signum == SIGUSR2) {
        if (is_restart) {
            puts("Restarting...");
            is_restart = 0;
        } else {
            puts("Checkpointing...");
            ckpt_fd = open(CKPT_FILE, O_WRONLY | O_CREAT |
                           O_TRUNC | O_APPEND, S_IWUSR);
            if (ckpt_fd < 0) {
                perror("open");
                exit(EXIT_FAILURE);
            }
            int i;
            if ((i = proc_self_maps(ckpt_segments)) == -1)
                exit(EXIT_FAILURE);
            if (save_context(&ckpt_segments[i]) == -1)
                exit(EXIT_FAILURE);
            if (write_ckpt(ckpt_fd, i + 1, ckpt_segments, &uc) == -1)
                exit(EXIT_FAILURE);
            is_restart = 1;
        }
    }
}

void
__attribute__((constructor)) init()
{
    signal(SIGUSR2, sig_handler);
}
