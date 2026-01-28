/* matrix.c */
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static int w = 0, h = 0;

void
display_matrix(int matrix[w][h])
{
    puts("Matrix:");
    for (int i = 0; i < h; ++i) {
        for (int j = 0; j < w; ++j) {
            printf("%-5d", matrix[i][j]);
        }
        printf("\n\n");
    }
}

int
main(int argc, char* argv[])
{
    if (argc < 2) {
        puts("Usage: ./matrix <interval length (us)> <matrix width/height>");
        exit(EXIT_FAILURE);
    }
    int interval = atoi(argv[1]);
    w = h = atoi(argv[2]);
    int matrix[w][h];
    memset(&matrix, 0, sizeof(matrix));
    for (int i = 0; i < h; i = ++i % h) {
        for (int j = 0; j < w; ++j) {
            system("clear");
            ++matrix[i][j];
            display_matrix(matrix);
            usleep(interval);
        }
    }
    exit(EXIT_SUCCESS);
}
