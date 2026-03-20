#define _GNU_SOURCE
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <ucontext.h>
#include <errno.h>
#include <err.h>
#include "testlib.h"

char *regs[NGREG] =
{
        "R8", "R9", "R10", "R11", "R12", "R13", "R14", "R15",
        "RDI", "RSI", "RBP", "RBX", "RDX", "RAX", "RCX", "RSP",
        "RIP", "EFL", "CSGSFS", "ERR", "TRAPNO", "OLDMASK", "CR2"
};

int readckpt(int fd, void *data, size_t sz)
{
        int rc, bytes = 0;

        while (bytes < sz) {
                rc = read(fd, data + bytes, sz - bytes);
                if (rc == 0)
                        return EOF;
                else if (rc < 0) {
                        perror("read");
                        return errno;
                }
                bytes += rc;
        }
        
        assert(bytes == sz);
        return 0;
}

int print(char *ckptfile)
{
        ckpthdr_t       hdrs[MAX_CKPTHDRS];
        regctx_t        ctxs[MAX_REGCTXS];
        memrgn_t        rgns[MAX_MEMRGNS];
        ucontext_t      uctx[MAX_REGCTXS];
        
        int rc, fd;
        int nr_ckpthdrs = 0, nr_regctxs = 0, nr_memrgns = 0;

        if ((fd = open(ckptfile, O_RDONLY)) < 0) {
                perror("open");
                return -1;
        }
        
        for (;;) {
                ckpthdr_t *h = hdrs + nr_ckpthdrs;
                size_t sz = sizeof(ckpthdr_t);
                
                rc = readckpt(fd, (void *)h, sz);

                if (rc != 0 && rc == EOF)
                        break;
                else if (rc != 0)
                        return -1;
                
                if (h->type & TY_REGCTX) {
                        regctx_t *c = ctxs + nr_regctxs;
                        sz = sizeof(regctx_t);

                        if (readckpt(fd, (void *)c, sz) != 0)
                                return -1;
                        
                        ucontext_t *uc = uctx + nr_regctxs;
                        sz = sizeof(ucontext_t);

                        if (readckpt(fd, (void *)uc, sz) != 0)
                                return -1;

                        nr_regctxs++;
                } else if (h->type & TY_MEMRGN) {
                        memrgn_t *r = rgns + nr_memrgns;
                        sz = sizeof(memrgn_t);

                        if (readckpt(fd, (void *)r, sz) != 0)
                                return -1;
                        
                        u64 rgn_size = (u64)(r->end - r->start);
                        if (lseek(fd, rgn_size, SEEK_CUR) < 0)
                                return -1;

                        nr_memrgns++;
                }

                nr_ckpthdrs++;
        }
        
        printf("Displaying memory regions...\n");
        for (int i = 0; i < nr_memrgns; i++) {
                printf("Memory region %d:\n", i);
                memrgn_t *r = rgns + i;
                printf("%lx-%lx %c%c%c%c %s\n",
                       (u64)r->start, (u64)r->end,
                       (r->rwxp & R) ? 'r' : '-',
                       (r->rwxp & W) ? 'w' : '-',
                       (r->rwxp & X) ? 'x' : '-',
                       (r->rwxp & P) ? 'p' : 's',
                       r->name);
        }
        
        printf("Displaying register contexts...\n");
        for (int i = 0; i < nr_regctxs; i++) {
                printf("Register context %d:\n", i);
                regctx_t *c = ctxs + i;
                ucontext_t *uc = uctx + i;
                for (int i = 0; i < NGREG; i++) {
                        printf("%-12s%p\n",
                               regs[i],
                               uc->uc_mcontext.gregs[i]);
                }
        }

        return 0;
}

int main(int argc, char *argv[])
{
        if (argc < 2) {
                fprintf(stderr, "Usage: ./print <ckpt-file>\n");
                exit(1);
        }

        if (print(argv[1]) < 0)
                exit(1);

        exit(0);
}
