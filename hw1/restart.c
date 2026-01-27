/* restart.c */
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>

int
main(int argc, char *argv[])
{
    setenv("LD_PRELOAD", "./libckpt.so", 1);
    if (argc < 1) {
        puts("Usage: ./restart <checkpoint image file>");
        exit(EXIT_FAILURE);
    }
    
    return EXIT_SUCCESS;
}
