#define _GNU_SOURCE
#include <ucontext.h>
#include <stdio.h>
#include <stdlib.h>

#define STACK_PTR       "$sp"
#define PROGRAM_COUNTER "$pc"

#ifdef __aarch64__
#   define NUM_REGS     31
#   define REGS         uc_mcontext.regs
#   define SP           uc_mcontext.sp
#   define PC           uc_mcontext.pc
#   define SYS          "aarch64"
static char *regs[] = {
    "X0", "X1", "X2", "X3", "X4", "X5", "X6", "X7", "X8", "X9",
    "X10", "X11", "X12", "X13", "X14", "X15", "X16", "X17", "X18",
    "X19", "X20", "X21", "X22", "X23", "X24", "X25", "X26", "X27",
    "X28", "FP", "LR"
};
#elif defined __x86_64
#   define NUM_REGS     23
#   define REGS         uc_mcontext.gregs
#   define SP           uc_mcontext.gregs[15]
#   define PC           uc_mcontext.gregs[16]
#   define SYS          "x86_64"
static char *regs[] = {
    "R8", "R9", "R10", "R11", "R12", "R13", "R14", "R15",
    "RDI", "RSI", "RBP", "RBX", "RDX", "RAX", "RCX", "RSP",
    "RIP", "EFL", "CSGSFS", "ERR", "TRAPNO", "OLDMASK", "CR2"
};
#endif

int
main(int argc, char *argv[])
{
    ucontext_t uc;
    if (getcontext(&uc) < 0) {
        perror("getcontext");
        exit(EXIT_FAILURE);
    }
    printf("CPU: %s\n", SYS);
    printf("# Registers: %d\n", NUM_REGS);
#ifdef __aarch64__
    printf("%-8s%p\n", STACK_PTR, (void *)uc.SP);
    printf("%-8s%p\n", PROGRAM_COUNTER, (void *)uc.PC);
#endif
    for (int i = 0; i < NUM_REGS; ++i) {
        if ((void *)uc.REGS[i] == NULL) 
            continue;
        printf("%-8s%p\n", regs[i], (void *)uc.REGS[i]);
    }
}
