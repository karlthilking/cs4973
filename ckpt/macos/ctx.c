#define _XOPEN_SOURCE
#include <ucontext.h>
#include <signal.h>
#include <unistd.h>
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

static ucontext_t uc;

void sighandler(int sig)
{
    static int is_restart;
    
    if (sig == SIGUSR2) {
        is_restart = 0;
        getcontext(&uc);

        if (is_restart) {
            printf("Restarting...\n");
            is_restart = 0;
        } else {
            printf("Checkpointing...\n");
            is_restart = 1;
            setcontext(&uc);
        }
    }

    return;
}

/**
 * raise(SIGUSR2) -> goto sighandler
 * 
 * sighandler:
 *  pacibsp: Pointer authentication code
 *  ...
 *  getcontext(&uc)
 *  is_restart = 1  
 *  ...
 *  retab: Return and authenticate pointer
 *
 * setcontext(&uc) -> goto where getcontext() was called
 *  is_restart = 0
 *  ...
 *  retab: Return and authenticate pointer (now corrupted)
 *  SIGSEGV
 */

int main(void)
{
    signal(SIGUSR2, sighandler);
    raise(SIGUSR2);

    printf("Executing rest of program...\n");

    return 0;
}

