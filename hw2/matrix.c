/* matrix.c */
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

void
display_matrix(int matrix[5][5])
{
    puts("Matrix:");
    for (int i = 0; i < 5; ++i) {
        printf("%-4d%-4d%-4d%-4d%d\n",
               matrix[i][0], matrix[i][1],
               matrix[i][2], matrix[i][3],
               matrix[i][4]);
    }
}

int
main(int argc, char* argv[])
{
    if (argc < 2) {
        puts("Usage: ./matrix <microsecond interval>");
        exit(EXIT_FAILURE);
    }
    int interval = atoi(argv[1]);
    int matrix[5][5];
    memset(&matrix, 0, sizeof(matrix));
    for (int i = 0; i < 5; i = ++i % 5) {
        for (int j = 0; j < 5; ++j) {
            system("clear");
            ++matrix[i][j];
            display_matrix(matrix);
            usleep(interval);
        }
    }
    exit(EXIT_SUCCESS);
}
