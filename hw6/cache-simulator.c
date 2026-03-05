#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <err.h>
#include <stdint.h>

typedef struct {
    unsigned long addr;
    unsigned char m;
    unsigned char v;
    unsigned int use;
} cache_line_t;

int
main(int argc, char *argv[])
{
    if (argc < 5) {
        fprintf(stderr, "Usage: ./cache-simulator --data-block-size [NUMBER]"
                        " --cache-size [NUMBER]\n");
        _exit(1);
    }

    const uint8_t data_block_size = stoi(argv[2]);
    const uint16_t cache_size = stoi(argv[4]);
    const uint16_t num_cache_lines = cache_size / data_block_size;

    cache_line_t cache_lines[num_cache_lines];
    for (int i = 0; i < num_cache_lines; ++i)
        cache_lines[i].v = 0x0; // mark each cache line as invalid initially

    char *line = NULL;
    size_t sz = 0;
    char rw;
    unsigned long addr;
    
    int iter = 0;
    while (getline(&line, &sz, stdin) > 0 && !feof(stdin)) {
        if (sscanf(line, "%c:%lx\n", &rw, &addr) < 2)
            err(EXIT_FAILURE, "sscanf");
        int i;
        for (i = 0; i < num_cache_lines; ++i) {
            /* cache hit */
            if (!cache_lines[i].v) {
                cache_lines[i].addr = addr;
                cache_lines[i].v = 0x1;
                cache_lines[i].m = 0x0;
                cache_lines[i].use = iter++;
                break;
            } else if (cache_lines[i].addr == addr && cache_lines[i].v)
                if (rw == 'r') {
                    printf("Read from %lx\n", addr);
                } else if (rw == 'w') {
                    if (!cache_lines[i].m)
                        cache_lines[i].m = 0x1;
                    printf("Wrote to %lx\n", addr);
                }
                cache_lines[i].use = iter++;
                break;
            }

        }
        /* cache miss, need to evict a cache line now */
        if (i >= num_cache_lines) {
            int lru = INT_MAX, lru_ix = -1;
            for (i = 0; i < num_cache_lines; ++i) {
                if (cache_lines[i].use < lru) {
                    lru = cache_lines[i].use;
                    lru_ix = i;
                }
            }
            cache_lines[lru_ix].addr = addr;
            cache_lines[lru_ix].use = iter++;
            cache_lines[lru_ix].m = 0x0;
            cache_lines[lru_ix].v = 0x1;
        }
    }
    _exit(0);
}
