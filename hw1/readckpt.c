/* readckpt.c */
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

// ucontext_t contains a field uc_mcontext of type mcontext_t 
// that contains the general registers in a 'gregset_t' array
// according the glibc sysdeps/x86_64/sys/ucontext.h,
// the registers are arranged in this order
static char *regs[] = {
    "R8", "R9", "R10", "R11", "R12", "R13", "R14", "R15",
    "RDI", "RSI", "RBP", "RBX", "RDX", "RAX", "RCX", "RSP",
    "RIP", "EFL", "CSGSFS", "ERR", "TRAPNO", "OLDMASK", "CR2"
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
read_ckpt_file(int ckpt_fd, struct ckpt_segment ckpt_segments[],
                   ucontext_t *ucp)
{
    int i, rc, tmp, bytes_read;
    char data_buf[BUF_SIZE];
    i = 0;
    while (1) {
        memset(data_buf, 0, BUF_SIZE);
        rc = 0;
        while (rc < sizeof(struct ckpt_segment)) {
            if ((tmp = pread(ckpt_fd, &ckpt_segments[i] + rc, 
                             sizeof(struct ckpt_segment) - rc, 
                             bytes_read + rc)) < 0) {
                perror("pread");
                return -1;
            }
            rc += tmp;
        }
        assert(rc == sizeof(struct ckpt_segment));
        bytes_read += rc;
        if (ckpt_segments[i++].is_reg_context == 1) 
            break;
    }
    memset(data_buf, 0, BUF_SIZE);
    rc = 0;
    while (rc < sizeof(*ucp)) {
        if ((tmp = pread(ckpt_fd, ucp + rc, 
                         sizeof(*ucp) - rc, 
                         bytes_read + rc)) < 0) {
            perror("pread");
            return -1;
        }
        rc += tmp;
    }
    assert(rc == sizeof(*ucp));
    bytes_read += rc;
    return 0;
}

void
print_ckpt_segments(struct ckpt_segment ckpt_segments[])
{
    int i;
    for (i = 0; ckpt_segments[i].is_reg_context == 1 || 
                ckpt_segments[i].start != NULL; ++i) {
        printf("%p-%p %c%c%c%c %s %d %zu\n",
               ckpt_segments[i].start, ckpt_segments[i].end,
               ckpt_segments[i].rwxp[0], ckpt_segments[i].rwxp[1],
               ckpt_segments[i].rwxp[2], ckpt_segments[i].rwxp[3],
               ckpt_segments[i].name, ckpt_segments[i].is_reg_context,
               ckpt_segments[i].data_size);
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
    struct ckpt_segment ckpt_segments[MAX_CKPT_SEGMENTS];
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
    getcontext(&uc);
    print_ucontext_regs(&uc);
    return EXIT_SUCCESS;
}
