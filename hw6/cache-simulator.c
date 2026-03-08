#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <limits.h>
#include <err.h>
#include <assert.h>
#include <string.h>

#define M   0x4 // modified bit
#define V   0x2 // valid bit
#define U   0x1 // use bit

typedef struct cache_line {
    // example: 64 byte cache lines, 64 bit address, fully associative
    // -> 6 offset bits, 0 index bits (fully associative), 58 tag bits
    // -> lower 3 bits used for modifed, valid, used and upper 58 for tag
    alignas(64) uint64_t hdr;
} cache_line_t;

// calculate log base 2 for integer type
int
log2i(int x)
{
    int ret;
    for (ret = 0; x > 1; x >>= 1, ++ret)
        ;
    return ret;
}

//  verbose mode: 
//  - report hit or miss for each load or store
//  - report evictions and if they had to be written back
//
// params:
// - array of cache_lines 
// - cache line size
// - cache size
// - number of cache lines
int
run_simulation(cache_line_t *cache_lines, const uint8_t cache_line_size, 
               const uint16_t cache_size, const uint16_t n_cache_lines)
{
    // number of bits for extra info: modified, valid, used
    const uint8_t n_infobits = 3;
    // number of offset bits = log2(block_size)
    const uint8_t n_offbits = log2i(cache_line_size);
    // number of tag bits = 64 - offset bits - information bits (m, v, u)
    const uint8_t n_tagbits = 64 - n_offbits - 3;
    
    // initialize mask for offset bits and tag bits
    const uint64_t OFFBITMASK = (uint64_t)((1 << n_offbits) - 1);
    const uint64_t TAGBITMASK = (uint64_t)(~OFFBITMASK);
    
    // keep track of hits and misses
    int n_hits = 0, n_misses = 0;

    char            mode;   // read or write
    unsigned long   addr;   // address (64 bit) loaded or stored to
    
    while (!feof(stdin)) {
        if (scanf("%c%lx\n", &mode, &addr) < 2)
            return -1;
        int i;
        for (i = 0; i < n_cache_lines; ++i) {
            // if tag bits match and valid bit is set -> cache hit
            if ((cache_lines[i].hdr >> n_infobits) == (addr >> n_offbits) &&
                (cache_lines[i].hdr & V)) {
                assert(mode == 'r' || mode == 'w');
                printf("cache hit: ");
                cache_lines[i].hdr |= U; // set used bit
                if (mode == 'r')
                    printf("read to %lx\n", addr);
                else {
                    printf("write to %lx\n", addr);
                    cache_lines[i].hdr |= M; // set modified bit after store
                }
                ++n_hits;
                break; // found address in cache; break from loop
            }
        }
        // i == n_cache_lines (traversed all lines), then cache miss
        // either replace an invalid line or evict a line if all are valid
        if (i == n_cache_lines) {
            ++n_misses;
            printf("cache miss: ");
            for (i = 0;; i = (i + 1) % n_cache_lines) {
                // found invalid cache line (free to replace)
                if (!(cache_lines[i].hdr & V)) {
                    assert(mode == 'r' || mode == 'w');
                    // set cache line tag bits to correspondig tag of
                    // address, set valid bit, set use bit
                    cache_lines[i].hdr = (addr >> (n_offbits - n_infobits));
                    cache_lines[i].hdr |= V;
                    cache_lines[i].hdr |= U;
                    if (mode == 'r')
                        printf("read to %lx, ", addr);
                    else {
                        printf("write to %lx, ", addr);
                        cache_lines[i].hdr |= M;
                    }
                    printf("replaced invalid line\n");
                    break; // replaced invalid line, break from loop
                } else if (!(cache_lines[i].hdr & U)) {
                    assert(mode == 'r' || mode == 'w');
                    // save evicted cache line info
                    uint64_t evicted = cache_lines[i].hdr;
                    
                    cache_lines[i].hdr = (addr >> (n_offbits - n_infobits));
                    cache_lines[i].hdr |= V;
                    cache_lines[i].hdr |= U;
                    if (mode == 'r')
                        printf("read to %lx\n", addr);
                    else {
                        printf("write to %lx\n", addr);
                        cache_lines[i].hdr |= M;
                    }
                    printf("evicted line at %lx, ", 
                           (evicted >> n_infobits) << n_offbits);
                    if (evicted & M)
                        printf("writing back... (modified)\n");
                    else
                        printf("no write back (unmodified)\n");
                    break;
                } else {
                    // if line is valid and use bit is set, unset use bit
                    // for clock eviction
                    cache_lines[i].hdr &= ~U;
                }
            }
        }
    }
    int n_total = n_hits + n_misses;
    // todo: display info about total hits and misses + other summary info
    return 0;
}

int
main(int argc, char *argv[])
{
    if (argc < 5) {
        fprintf(stderr, "Usage: ./cache-simulator --data-block-size [NUMBER]"
                        " --cache-size [NUMBER]\n");
        exit(1);
    }

    uint8_t cache_line_size;
    uint16_t cache_size, n_cache_lines;

    assert(strcmp(argv[1], "--data-block-size") == 0);
    assert(strcmp(argv[3], "--cache-size") == 0);

    cache_line_size = atoi(argv[2]);
    cache_size      = atoi(argv[4]);
    n_cache_lines   = cache_size / cache_line_size;
    
    cache_line_t cache_lines[n_cache_lines];
    for (int i = 0; i < n_cache_lines; ++i) {
        cache_lines[i].hdr &= ~V; // unset all valid bits
        assert(!(cache_lines[i].hdr & V));
    }

    if (run_simulation(cache_lines, cache_line_size, 
                       cache_size, n_cache_lines) < 0) {
        fprintf(stderr, "Error: verbose mode\n");
        exit(1);
    }

    exit(0);
}
