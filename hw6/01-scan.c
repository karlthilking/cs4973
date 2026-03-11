#include <stdlib.h>
#include <assert.h>

#define CACHE_SIZE 4096
#define BLOCK_SIZE 128
#define N          (CACHE_SIZE + BLOCK_SIZE)

static int A[N];

int
main(int argc, char *argv[])
{
        /**
         * Allocate array large enough to overwhelm to capacity of the cache
         * in order to test LRU-approximation corner case behavior
         */
        const int n_epochs = (argc > 1) ? atoi(argv[1]) : 1 << 12;
        const int step     = BLOCK_SIZE / sizeof(int);
        
        for (int epoch = 0; epoch < n_epochs; epoch++) {
                for (int i = 0; i < N; i += step) {
                        int x = A[i];
                        x++;
                }
        }
        
        exit(0);
}
