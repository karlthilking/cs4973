/* readckpt.c */
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <assert.h>
#include <signal.h>
#include <string.h>

#define NAME_LEN 80
#define UCONTEXT_SIZE sizeof(ucontext_t)

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
read_ckpt_segment(int ckpt_fd)
{
    struct ckpt_segment ckpt_segment;
    int tmp_stdin, rc;
    tmp_stdin = dup(0);
    dup2(ckpt_fd, 0);
    rc = scanf("%p-%p %s %s %d %zu\n",
                ckpt_segment.start, ckpt_segment.end, 
                ckpt_segment.rwxp, ckpt_segment.name,
                ckpt_segment.is_reg_context, 
                ckpt_segment.data_size);
    clearerr(stdin);
    dup2(tmp_stdin, 0);
    close(tmp_stdin);
    if (rc == 0 || rc == EOF) {
        return EOF;
    } else {
        if (ckpt_segment.is_reg_context == 1) {
            ucontext_t uc;
            char ucontext_data[UCONTEXT_SIZE];
            int rc2, tmp;
            rc2 = 0;
            while (rc2 < UCONTEXT_SIZE) {
                if ((tmp = read(ckpt_fd, ucontext_data, 
                                UCONTEXT_SIZE - rc2)) == -1) {
                    perror("read");
                    return EOF;
                }
                rc2 += tmp;
            }
            memcpy((void *)&uc, (void *)ucontext_data, UCONTEXT_SIZE);
            int i;
            for (i = 0; i < NGREG; ++i)
                printf("%s: %p\n", regs[i], (void *)uc.uc_mcontext.gregs[i]);
        } else {
            assert(rc == 6);
            ckpt_segment.rwxp[4] = '\0';
            ckpt_segment.name[NAME_LEN] = '\0';
            printf("%p-%p %s %s\n",
                    ckpt_segment.start, ckpt_segment.end,
                    ckpt_segment.rwxp, ckpt_segment.name);
        }
    }
    return 0;
}

int 
main(int argc, char *argv[])
{
    if (argc < 2) {
        puts("Usage: ./readckpt <checkpoint image file>\n");
    }
    char *ckpt_file = argv[1];
    int ckpt_fd, rc;
    if ((ckpt_fd = open(ckpt_file, O_RDONLY)) == -1) {
        perror("open");
        exit(EXIT_FAILURE);
    }
    while (rc != EOF)
        rc = read_ckpt_segment(ckpt_fd);
    return EXIT_SUCCESS;
}
