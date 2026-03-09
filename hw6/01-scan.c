#include <time.h>
#include <stdlib.h>

#define CACHE_SIZE 4096
#define BLOCK_SIZE 128

static int A[CACHE_SIZE + BLOCK_SIZE];

int
main(int argc, char *argv[])
{
    srand(time(NULL));
    /*
     *  Allocate array which is the size of the cpu cache plus one cache line
     *  and scan through array in order to invoke lru corner case behavior
     */
    const int n_epochs      = (argc > 1) ? atoi(argv[1]) : 100;
    const int n_cache_lines = CACHE_SIZE / BLOCK_SIZE;
    
    const int n_iterations  = CACHE_SIZE / sizeof(int);
    const int step          = BLOCK_SIZE / sizeof(int);
    
    for (int epoch = 0; epoch < n_epochs; ++epoch)
        for (int i = 0; i < n_iterations; i += step)
            A[i]++;

    exit(0);
}
