/* 04-random-access.c: Test random access pattern cache behavior */
#include <time.h>
#include <stdlib.h>

int A[4][1 << 11];

int main(int argc, char *argv[])
{
        srand(time(NULL));
        const int n_iterations = (argc > 1) ? atoi(argv[1]) : 1 << 14;
        for (int i = 0; i < n_iterations; i++)
                A[rand() % 4][rand() % (1 << 11)] = 0;
        exit(0);
}
