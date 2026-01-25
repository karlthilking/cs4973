/* readckpt.c */
#define _GNU_SOURCE
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <assert.h>
#include <signal.h>
#include <string.h>
#include <ucontext.h>

#define NAME_LEN            128
#define CKPT_FILE           "ckpt.dat"
#define BUF_SIZE            1024
#define MAX_CKPT_SEGMENTS   1000
#define CKPT_SEGMENT_SIZE   sizeof(ckpt_segment_t)
#define UCONTEXT_SIZE       sizeof(ucontext_t)

// ucontext_t contains a field uc_mcontext of type mcontext_t 
// that contains the general registers in a 'gregset_t' array
// according the glibc sysdeps/x86_64/sys/ucontext.h,
// the registers are arranged in this order
static char *regs[] = {
    "R8", "R9", "R10", "R11", "R12", "R13", "R14", "R15",
    "RDI", "RSI", "RBP", "RBX", "RDX", "RAX", "RCX", "RSP",
    "RIP", "EFL", "CSGSFS", "ERR", "TRAPNO", "OLDMASK", "CR2"
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
read_ckpt_file(int ckpt_fd, ckpt_segment_t ckpt_segments[], 
               ucontext_t *ucp)
{
    int rc, tmp, i;
    for (i = 0; ; ++i) {
        rc = 0;
        while (rc < CKPT_SEGMENT_SIZE) {
            if ((tmp = read(ckpt_fd, &ckpt_segments[i] + rc,
                            CKPT_SEGMENT_SIZE - rc)) < 0) {
                perror("read");
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
        if ((tmp = read(ckpt_fd, ucp + rc, UCONTEXT_SIZE - rc)) < 0) {
            perror("read");
            return -1;
        }
        rc += tmp;
    }
    assert(rc == UCONTEXT_SIZE);
    return 0;
}

void
print_ckpt_segments(ckpt_segment_t ckpt_segments[])
{
    int i;
    for (i = 0; ; ++i) {
        printf("%p-%p %c%c%c%c %s %d %d\n",
               ckpt_segments[i].start, ckpt_segments[i].end,
               ckpt_segments[i].rwxp[0], ckpt_segments[i].rwxp[1],
               ckpt_segments[i].rwxp[2], ckpt_segments[i].rwxp[3],
               ckpt_segments[i].name, 
               ckpt_segments[i].is_reg_context,
               ckpt_segments[i].data_size);
        if (ckpt_segments[i].is_reg_context)
            break;
    }
}

void
print_ucontext_regs(ucontext_t *ucp)
{
    int i;
    for (i = 0; i < NGREG; ++i) 
        printf("%s: %p\n", regs[i], ucp->uc_mcontext.gregs[i]);
}

int 
main(int argc, char *argv[])
{
    int ckpt_fd;
    ckpt_segment_t ckpt_segments[MAX_CKPT_SEGMENTS];
    ucontext_t uc;
    if ((ckpt_fd = open(CKPT_FILE, O_RDONLY, S_IRUSR)) == -1) {
        perror("open");
        exit(EXIT_FAILURE);
    }
    if (read_ckpt_file(ckpt_fd, ckpt_segments, &uc) == -1) {
        exit(EXIT_FAILURE);
    }
    print_ckpt_segments(ckpt_segments);
    print_ucontext_regs(&uc);
    close(ckpt_fd);
    return EXIT_SUCCESS;
}
