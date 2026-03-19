#define _GNU_SOURCE
#include <fcntl.h>
#include <dlfcn.h>
#include <sys/syscall.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <err.h>

#define SYS_open 2

static int n_opens;
int (*real_open)(const char *, int flags, .../* mode_t mode */);
void *libc_handle = NULL;

int open(const char *pathname, int flags, ...)
{
        int rc;

        va_list ap;
        va_start(ap, flags);

        if (!(flags & O_CREAT))
                rc = syscall(SYS_open, pathname, flags);
        else {
                mode_t mode = va_arg(ap, typeof(S_IRUSR));
                rc = syscall(SYS_open, pathname, flags, mode);
        }

        printf("Number of calls to open: %d\n", ++n_opens);

        va_end(ap);
        return rc;
}

void __attribute__((constructor)) setup()
{
        n_opens = 0;
        libc_handle = dlopen("libc.6.so", RTLD_LAZY);
        if (!libc_handle)
                err(EXIT_FAILURE, "dlopen");
}

