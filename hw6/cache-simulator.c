/* cache-simulator.c
 *  Fully-Associative Cache
 *  LRU Eviction
 *  'M' and 'V' bits (modified and valid)
 */

#include <stdlib.h>
#include <string.h>

#define CACHEREAD   0x0
#define CACHEWRITE  0x1

// void
// emitWrite(void *addr, unsigned char rw)
// {
//     
// }

void
emit_addr(void *addr, int rw)
{
    printf("%s %p\n", (rw == CACHEREAD) ? "Read from" : "Write to", addr);
}

int
main(int argc, char *argv[])
{
    unsigned int data_block_size = 0, cache_size = 0;
    for (int i = 1; i < argc; ++i) {
        if (!strncmp(argv[i], "--data-block-size", strlen("--data-block-size")))
            data_block_size = stou(argv[++i]);
        else if (!strncmp(argv[i], "--cache-size", strlen("--cache-size")))
            cache_size = stou(argv[++i]);
    }
    if (!(data_block_size && cache_size)) {
        fprintf(stderr, "a data block size and cache size must be specified\n");
        exit(1);
    } else if ((data_block_size >> 1) != (data_block_size / 2)) {
        fprintf(stderr, "data block size must be a power of two\n");
        exit(1);
    } else if ((cache_size >> 1) != (cache_size / 2)) {
        fprintf(stderr, "cache size must be a power of two\n");
        exit(1);
    }
    exit(0);
}
