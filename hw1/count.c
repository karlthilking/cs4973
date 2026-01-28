#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <limits.h>

int
main(int argc, char *argv[])
{
    int start = (argc > 1) ? atoi(argv[1]) : 0;
    while (start <= INT_MAX) {
        printf("%d ", start++);
        fflush(stdout);
        sleep(1);
    }
    return EXIT_SUCCESS;
}
