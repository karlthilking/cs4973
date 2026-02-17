/* inode.c */
#include <sys/stat.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>

int
main(int argc, char *argv[])
{
    struct stat statbuf;
    if (argc < 2) {
        fprintf(stderr, "Usage: ./inode [pathname]\n");
        exit(1);
    }
    if (stat(argv[1], &statbuf) < 0) {
        perror("stat");
        exit(1);
    }
    printf("%s Inode Number: %ld\n", argv[1], statbuf.st_ino);
    exit(0);
}
