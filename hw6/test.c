#include <stdlib.h>
#include <time.h>

int
main(void)
{
    srand(time(NULL));
    int *A, *B, *C;
    A = (int *)malloc(sizeof(int) * (1 << 10));
    B = (int *)malloc(sizeof(int) * (1 << 10));
    C = (int *)calloc(1 << 10, sizeof(int));

    for (int i = 0; i < (1 << 5); ++i) {
        for (int j = 0; j < (1 << 5); ++j) {
            *(A + (i * (1 << 5)) + j) = rand() % (1 << 10);
            *(B + (i * (1 << 5)) + j) = rand() % (1 << 10);
        }
    }
}
