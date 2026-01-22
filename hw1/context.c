#include <ucontext.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <unistd.h>

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

int print_proc_self_maps()
{
    char sys_call[80];
    pid_t pid = getpid();
    sprintf(sys_call, "cat /proc/%d/maps\n", pid);
    sys_call[79] = '\0';
    printf("Executing: %s\n", sys_call);
    return system(sys_call); 
}

int main(int argc, char *argv[])
{
    ucontext_t uc;
    if (getcontext(&uc) == -1) {
        perror("getcontext");
        exit(EXIT_FAILURE);
    }
    if (print_proc_self_maps() == -1) {
        perror("system");
        exit(EXIT_FAILURE);
    }
#ifdef __x86_64__
    puts("Architecture: x86_64\n");
#else
    puts("Architecture: x86_32\n");
#endif
    printf("# General Registers: %d\n", NGREG);
    printf("uc.uc_stack.ss_sp: %p\n", uc.uc_stack.ss_sp);
    printf("uc.uc_mcontext.gregs[15]: %p\n", uc.uc_mcontext.gregs[15]);
    printf("uc.uc_stack.ss_size: %zu\n", (void *)uc.uc_stack.ss_size);
    printf("stack size: %p\n", (void *)uc.uc_stack.ss_size - (void *)uc.uc_mcontext.gregs[15]);
    int i;
    for (i = 0; i < NGREG; ++i)
        printf("%s: %p\n", regs[i], (void *)uc.uc_mcontext.gregs[i]);
    return EXIT_SUCCESS;
}
