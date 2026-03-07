#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <limits.h>
#include <err.h>
#include <math.h>
#include <assert.h>

typedef struct cache_line_hdr {
    unsigned long   tag : 61;   // tag bits
    unsigned int    m   : 1;    // modified bit
    unsigned int    v   : 1;    // valid bit
    unsigned int    u   : 1;    // use bit
} cache_line_hdr_t;

typedef struct cache_line {
    cache_line_hdr_t    hdr;
    char                data[CACHE_LINE_SIZE];
} cache_line_t;

/* cache parameters */
uint8_t     cache_line_size;
uint16_t    cache_size;
uint16_t    n_cache_lines;

/* log2 for integer types */
int
log2i(int x)
{
    int ret;
    for (ret = 0; x > 1; x >>= 1, ++ret)
        ;
    return ret;
}

int
verbose(cache_line_t *cache_lines)
{
    uint8_t     n_offbits;  // number of offset bits
    uint8_t     n_tagbits;  // number of tag bits
    uint64_t    tagbitmask; // mask for tag bits in address
    
    n_offbits   = log2i(cache_line_size);   // log2(block size) offset bits
    n_tagbits   = 64 - n_offbits;           // rest of bits used for tag
    tagbitmask  = ~((1 << n_offbits) - 1)

    char            rw;     // read or write
    unsigned long   addr;   // generated virtual address
    
    while (!feof(stdin)) {
        if (scanf("%c%lx\n", &rw, &addr) < 2)
            return -1;
        int i;
        for (i = 0; i < n_cache_lines; ++i) {
            /* cache hit */
            if ((cache_lines[i].tag << n_offbits) == (addr & tagbitmask) &&
                cache_lines[i].v) {
                assert(rw == 'r' || rw == 'w');
                printf("Cache Hit: ");
                cache_lines[i].u = 1; // set use bit
                if (rw == 'r')
                    printf("Read from %lx\n", addr);
                else {
                    printf("Write to %lx\n", addr);
                    cache_lines[i].m = 1; // set modified bit if writing
                }
                break;
            }
        }
        /* cache miss */
        if (i == n_cache_lines) {
            for (i = 0;; i = (i + 1) % n_cache_lines) {
                if (!(cache_lines[i].v && cache_lines[i].u))  {
                    // evict first cache line that is invalid or does not have
                    // its reference/use bit set
                    assert(rw == 'r' || rw == 'w');
                    // if the replaced cache line was valid, print that its
                    // being evicted and also if it needs to be written back
                    if (cache_lines[i].v) {
                        printf("Evicting line at %lx: ", 
                                cache_lines[i].tag << n_offbits);
                        if (cache_lines[i].m)
                            printf("writing back (modified)\n");
                        else
                            printf("no write back (unmodified)\n");
                    }
                    cache_lines[i].tag = addr >> n_offbits;
                    cache_lines[i].v = 1;
                    cache_lines[i].u = 1;
                    printf("Cache Miss: ");
                    if (rw == 'r')
                        printf("Read from %lx\n", addr);
                    else {
                        printf("Write to %lx\n", addr);
                        cache_lines[i].m = 1;
                    }
                    break;
                } else if (cache_lines[i].u) {
                    // if use bit is set, unset during eviction
                    cache_lines[i].u = 0;
                }
            }
        }
    }
    return 0;
}

int
summary(const cache_line_t *cache_lines)
{

    return 0;
}

int
main(int argc, char *argv[])
{
    if (argc < 5) {
        fprintf(stderr, "Usage: ./cache-simulator --data-block-size [NUMBER]"
                        " --cache-size [NUMBER] --mode [verbose | summary]\n");
        exit(1);
    }
    
    cache_line_size = atoi(argv[2]);
    cache_size      = atoi(argv[4]);
    n_cache_lines   = cache_size / cache_line_size;
    
    // allocate N cache lines and unset all valid bits
    cache_line_t cache_lines[n_cache_lines];
    for (int i = 0; i < n_cache_lines; ++i)
        cache_lines[i].v = 0;
    
    char mode = argv[6][0];
    if (mode == 'v') {
        if (verbose(cache_lines) < 0)
            err(EXIT_FAILURE, "cache-simulator (verbose mode)");
    } else if (mode == 's') {
        if (summary(cache_lines) < 0)
            err(EXIT_FAILURE, "cache-simulator (summary mode)");
    } else {
        fprintf(stderr, "Unrecognized Mode: %s\n", argv[6]);
        exit(1);
    }
    exit(0);
}
