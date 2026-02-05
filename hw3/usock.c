#include <sys/socket.h>
#include <stdlib.h>
#include <unistd.h>
#include <err.h>
#include <errno.h>
#include <sys/un.h>
#include <string.h>
#include <stdio.h>
#include <signal.h>
#include <fcntl.h>

#define WR 0
#define RD 1

int
main(int argc, char *argv[])
{
    int errsv, sv[2];
    char buf[256];
    memset(buf, 0, sizeof(buf));
    if (socketpair(AF_UNIX, SOCK_STREAM, 0, sv) < 0)
        err(EXIT_FAILURE, "socketpair");
    switch (fork()) {
        case -1:
            err(EXIT_FAILURE, "fork");
        case 0:
            errsv = EBADF;
            if (write(sv[WR], (void *)&errsv, sizeof(errsv)) < 0)
                err(EXIT_FAILURE, "write");
            break;
        default:
            if (read(sv[RD], (void *)&errsv, sizeof(errsv)) < 0)
                err(EXIT_FAILURE, "read");
            fprintf(stderr, "%d: %s\n", errsv, strerror(errsv));
            break;
    }
    close(sv[WR]);
    close(sv[RD]);
    exit(0);
}
