#include <sys/socket.h>
#include <stdlib.h>
#include <unistd.h>
#include <err.h>
#include <errno.h>
#include <sys/un.h>
#include <string.h>
#include <stdio.h>
#include <signal.h>

#define WR 0
#define RD 1

int
main(int argc, char *argv[])
{
    int rc, tmp, fd, errsv, sv[2];
    char buf[256];
    memset(buf, 0, sizeof(buf));
    if (socketpair(AF_UNIX, SOCK_STREAM, 0, sv) < 0)
        err(EXIT_FAILURE, "socketpair");
    switch (fork()) {
        case -1:
            err(EXIT_FAILURE, "fork");
        case 0:
            if ((fd = dup(sv[WR])) < 0)
                err(EXIT_FAILURE, "dup");
            close(sv[WR]);
            close(sv[RD]);
            snprintf(buf, sizeof(buf), "errno: %d\n", errno);
            rc = 0;
            while (rc < sizeof(buf)) {
                tmp = write(fd, buf + rc, sizeof(buf) - rc);
                if (tmp < 0)
                    err(EXIT_FAILURE, "write");
                rc += tmp;
            }
            break;
        default:
            if ((fd = dup(sv[RD])) < 0)
                err(EXIT_FAILURE, "dup");
            close(sv[WR]);
            close(sv[RD]);
            rc = 0;
            while (rc < sizeof(buf)) {
                tmp = read(fd, buf + rc, sizeof(buf) - rc);
                if (tmp < 0)
                    err(EXIT_FAILURE, "read");
                rc += tmp;
            }
            rc = sscanf(buf, "%*s %d\n", &errsv);
            if (rc == 0 || rc == EOF)
                fprintf(stderr, "failed to read message\n");
            else
                printf("errno: %d\n", errsv);
            break;
    }
    if (fd != -1)
        close(fd);
    exit(0);
}
