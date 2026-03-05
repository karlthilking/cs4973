#include <stdio.h>
#include <stdlib.h>
#include <err.h>
#include <assert.h>

typedef struct {
    unsigned long   addr;   // start address held by cache line
    char            m;      // modified
    char            v;      // valid
    unsigned        lu;     // time of last use
} cache_line_t;

int
main(int argc, char *argv[])
{
    if (argc < 5) {
        fprintf(stderr, "Usage: ./cache-simulator --data-block-size [NUMBER]"
                        " --cache-size [NUMBER]\n");
        exit(1);
    }

    const uint8_t cache_line_size = stoi(argv[2]);
    const uint16_t cache_size = stoi(argv[4]);
    const uint16_t n_cache_lines = cache_size / cache_line_size;

    cache_line_t cache_lines[num_cache_lines];

    // mark each cache line as invalid initially
    for (int i = 0; i < num_cache_lines; ++i)
        cache_lines[v] = 0;
    
    char rw;
    unsigned long addr;
    int access = 0;

    while (!feof(stdin)) {
        /* get read-write specifying char and address used */
        if (scanf("%c%lx\n", &rw, &addr) < 2)
            err(EXIT_FAILURE, "scanf");
        /* keep track of lru value and index, or available invalid line */
        int i, avail = -1, lru = INT_MAX, evict = -1;
        for (i = 0; i < num_cache_lines; ++i) {
            /* cache hit */
            if (cache_lines[i].v && cache_lines[i].addr <= addr && 
                cache_lines[i].addr + cache_line_size >= addr) {
                assert(rw == 'r' || rw == 'w');
                printf("Cache Hit: ");
                if (rw == 'r') {
                    printf("Read from %lx\n", addr);
                } else if (rw == 'w') {
                    printf("Write to %lx\n", addr);
                    cache_lines[i].m = 1;
                }
                cache_lines[i].lu = access++;
                break;
            } else if (!cache_lines[i].v && avail == -1) {
                avail = i;
            } else if (avail == -1 && cache_lines[i].lu < lru) {
                lru = cache_lines[i].lu;
                evict = i;
            }
        }
        /* cache miss */
        if (i >= num_cache_lines) {
            /* 
             *  Either load data block into invalid cache line or replaced lru
             *  cache line with current
             */
            assert(avail != -1 || evict != -1);
            int index = (avail != -1) ? avail : evict;
            cache_lines[index].addr = addr;
            cache_lines[index].m = 0;
            cache_lines[index].v = 1;
            cache_lines[index].lu = access++;
            
            assert(rw == 'r' || rw == 'w');
            printf("Cache Miss: ");
            if (rw == 'r') {
                printf("Read from %lx\n", addr);
            } else if (rw == 'w') {
                printf("Write to %lx\n", addr);
            }
        }
    }

    exit(0);
}
