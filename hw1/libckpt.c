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
#define CKPT_FILE           "ckpt.dat"
#define BUF_SIZE            1024
#define MAX_CKPT_SEGMENTS   1000
#define CKPT_SEGMENT_SIZE   sizeof(ckpt_segment_t)
#define UCONTEXT_SIZE       sizeof(ucontext_t)

enum {
    VSYSCALL = -1,
    VDSO     = -2,
    VVAR     = -3
};

static char *regs[] = {
    "R8", "R9", "R10", "R11", "R12", "R13", "R14", "R15", "RDI",
    "RSI", "RBP", "RBX", "RDX", "RAX", "RCX", "RSP", "RIP",
    "EFL", "CSGSFS", "ERR", "TRAPNO", "OLDMASK", "CR2"
};

typedef struct {
    void *start;
    void *end;
    char rwxp[4];
    char name[NAME_LEN];
    int is_reg_context;
    int data_size;
} ckpt_segment_t;

int 
proc_self_maps_line(int proc_maps_fd, ckpt_segment_t *ckpt_segment)
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
    ckpt_segment->is_reg_context = 0;
    if (rc == 0 || rc == EOF) {
        ckpt_segment->start = NULL;
        ckpt_segment->end = NULL;
        return EOF;
    } else if (rc == 3) {
        strncpy(ckpt_segment->name, "ANONYMOUS_SEGMENT",
                strlen("ANONYMOUS_SEGMENT") + 1);
        ckpt_segment->name[strlen("ANONYMOUS_SEGMENT")] = '\0';
    } else {
        assert(rc == 4);
        ckpt_segment->name[NAME_LEN - 1] = '\0';
        if (strcmp(ckpt_segment->name, "[vsyscall]") == 0)
            return VSYSCALL; 
        else if (strcmp(ckpt_segment->name, "[vdso]") == 0)
            return VDSO;
        else if (strcmp(ckpt_segment->name, "[vvar]") == 0)
            return VVAR;
    }
    ckpt_segment->start = (void *)start;
    ckpt_segment->end = (void *)end;
    ckpt_segment->data_size = CKPT_SEGMENT_SIZE;
    return 0;
}

int 
proc_self_maps(ckpt_segment_t ckpt_segments[])
{
    int i, rc, proc_maps_fd;
    if ((proc_maps_fd = open("/proc/self/maps", 
                             O_RDONLY, S_IRUSR)) == -1) {
        perror("open");
        return -1;
    }
    for (i = 0; rc != EOF; ) {
        rc = proc_self_maps_line(proc_maps_fd, &ckpt_segments[i]);
        // if proc_self_maps_line returns a number less than 0
        // (indicator that is encounterd vsyscall, vdso, or vvar)
        // reset ckpt_segments[i] and don't increment i in order to 
        // avoid saving these segments
        if (rc < 0)
            memset(&ckpt_segments[i], 0, CKPT_SEGMENT_SIZE);
        else
            ++i;
    }
    close(proc_maps_fd);
    // i = next valid index in ckpt_segments[] array (for saving
    // the ucontext_t ckpt_segment)
    return i;
}

int 
save_context(ckpt_segment_t *ckpt_segment) 
{
    ckpt_segment->start = NULL;
    ckpt_segment->end = NULL;
    strncpy(ckpt_segment->rwxp, "----", 4);
    strncpy(ckpt_segment->name, "CONTEXT", strlen("CONTEXT") + 1);
    ckpt_segment->name[strlen("CONTEXT")] = '\0';
    ckpt_segment->is_reg_context = 1;
    ckpt_segment->data_size = UCONTEXT_SIZE;
    return 0;
}

int write_ckpt(int ckpt_fd, ckpt_segment_t ckpt_segments[], 
               ucontext_t *ucp)
{
    int rc, tmp, i;
    for (i = 0; ; ++i) {
        rc = 0;
        while (rc < CKPT_SEGMENT_SIZE) {
            if ((tmp = write(ckpt_fd, &ckpt_segments[i] + rc,
                             CKPT_SEGMENT_SIZE - rc)) < 0) {
                perror("write");
                return -1;
            }
            rc += tmp;
        }
        assert(rc == CKPT_SEGMENT_SIZE);
        if (ckpt_segments[i].is_reg_context == 1)
            break;
    }
    rc = 0;
    while (rc < UCONTEXT_SIZE) {
        if ((tmp = write(ckpt_fd, ucp + rc, UCONTEXT_SIZE - rc)) < 0) {
            perror("write");
            return -1;
        }
        rc += tmp;
    }
    assert(rc == UCONTEXT_SIZE);
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
    static int is_restart = 0;
    ckpt_segment_t ckpt_segments[MAX_CKPT_SEGMENTS];
    int ckpt_fd;
    if (signum == SIGUSR2) {
        if (is_restart) {
            puts("Restarting...");
            is_restart = 0;
        } else {
            puts("Checkpointing...");
            ckpt_fd = open(CKPT_FILE, O_WRONLY | O_CREAT |
                           O_TRUNC, S_IWUSR | S_IRUSR);
            if (ckpt_fd < 0) {
                perror("open");
                exit(EXIT_FAILURE);
            }
            int i;
            if ((i = proc_self_maps(ckpt_segments)) < 0)
                exit(EXIT_FAILURE);
            save_context(&ckpt_segments[i]);
            if (write_ckpt(ckpt_fd, ckpt_segments, &uc) < 0)
                exit(EXIT_FAILURE);
            close(ckpt_fd);
            is_restart = 1;
        }
    }
}

void
__attribute__((constructor)) init()
{
    signal(SIGUSR2, sig_handler);
}
