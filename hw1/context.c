#include <ucontext.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <unistd.h>
#include <fcntl.h>
#include <assert.h>

#define CONTEXT_FILE "context.dat"

// RIP: Program Counter
// RBP: Frame Pointer
// RSP: Stack Pointer
#ifdef __x86_64__
static char *regs[23] = {
    "R8", "R9", "R10", "R11", "R12", "R13", "R14", "R15",
    "RDI", "RSI", "RBP", "RBX", "RDX", "RAX", "RCX", "RSP",
    "RIP", "EFL", "CSGSFS", "ERR", "TRAPNO", "OLDMASK", "CR2"
};
#else
static char *regs[19] = {
    "GS", "FS", "ES", "DS", "EDI", "ESI", "EBP", "ESP",
    "EBX", "EDX", "ECX", "EAX", "TRAPNO", "ERR", "EIP",
    "CS", "EFL", "UESP", "SS"
};
#endif

void
print_context_info(ucontext_t *ucp)
{
    int i;
    for (i = 0; i < NGREG; ++i)
        printf("%s: %p\n", regs[i], ucp->uc_mcontext.gregs[i]);
}

int 
write_context(int fd, ucontext_t *ucp)
{
    int rc, tmp;
    rc = 0;
    while (rc < sizeof(*ucp)) {
        if ((tmp = write(fd, (char *)ucp + rc, sizeof(*ucp) - rc)) < 0) {
            perror("write");
            return -1;
        }
        rc += tmp;
    }
    assert(rc == sizeof(*ucp));
    return 0;
}

int
read_context(int fd, ucontext_t *ucp)
{
    int rc, tmp;
    rc = 0;
    while (rc < sizeof(*ucp)) {
        if ((tmp = pread(fd, ucp + rc, sizeof(*ucp) - rc, rc)) < 0) {
            perror("read");
            return -1;
        }
        rc += tmp;
    }
    return 0;
}

int main(int argc, char *argv[])
{
    ucontext_t uc1, uc2;
    getcontext(&uc1);
    int fd;
    if ((fd = open(CONTEXT_FILE, O_RDWR | O_CREAT | O_TRUNC,
                   S_IRUSR | S_IWUSR)) < 0) {
        perror("open");
        exit(EXIT_FAILURE);
    }
    if (write_context(fd, &uc1) < 0) 
        exit(EXIT_FAILURE);
    if (read_context(fd, &uc2) < 0)
        exit(EXIT_FAILURE);
    puts("Context 1:");
    print_context_info(&uc1);
    puts("Context 2:");
    print_context_info(&uc2);
    return EXIT_SUCCESS;
}
