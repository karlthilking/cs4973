/* ckpt.c */
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>

int main(int argc, char *argv[])
{
    setenv("LD_PRELOAD", "./libckpt.so", 1);
    if (argc < 1) {
        puts("Usage: ./ckpt <executable>\n");
        exit(EXIT_FAILURE);
    }
    execvp(argv[1], &argv[1]);
    return EXIT_SUCCESS;
}
