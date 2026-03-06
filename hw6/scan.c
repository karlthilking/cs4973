#include <time.h>
#include <stdlib.h>

int *A;

int
main(int argc, char *argv[])
{
    srand(time(NULL));
    /*
     *  Allocate array which is the size of the cpu cache plus one cache line
     *  and scan through array in order to invoke lru corner case behavior
     */
    const int n_epochs      = (argc > 1) ? atoi(argv[1]) : 10;
    const int block_size    = 128;
    const int cache_size    = 4096;
    const int n_cache_lines = cache_size / block_size;
    
    A = malloc(cache_size + block_size);
    
    const int n_iterations  = cache_size / sizeof(int);
    const int step          = block_size / sizeof(int);
    
    for (int epoch = 0; epoch < n_epochs; ++epoch)
        for (int i = 0; i < n_iterations; i += step)
            A[i] = rand() % 64;

    free(A);
    exit(0);
}
