#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <limits.h>
#include <err.h>
#include <assert.h>
#include <string.h>

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

/* Cache line in fully-associative cache */
typedef struct cache_line {
        u64 tag : 61;   // tag bits of address
        u8  m   : 1;    // modifed bit
        u8  v   : 1;    // valid bit
        u8  u   : 1;    // use bit
} cache_line_t;

typedef struct cache {
        cache_line_t    *cache_lines;
        u16             n_cache_lines;  // number of cache lines
        u8              n_offbits;      // number of offset bits per block
        u8              n_tagbits;      // number of tag bits per block
} cache_t;

/**
 * cache_get_offbits(): calculate the number of offset bits needed 
 * @block_size: size in byes of each cache data block
 */
u8 cache_get_offbits(u8 block_size)
{
        /* the number of offset bits = log2(data_block_size) */
        u8 n_offbits;
        for (n_offbits = 0, block_size > 1; block_size >>= 1; ++n_offbits)
                ;
        return n_offbits;
}

/**
 *cache_create(): initialize the cache structure
 * @cache_size: data size of the cache in bytes
 * @block_size: size of each data block/cache line in bytes
 */
cache_t *cache_create(u16 cache_size, u8 block_size)
{
        cache_t *cache = malloc(sizeof(*cache));
        cache->n_cache_lines = cache_size / block_size;
        cache->n_offbits = cache_get_offbits(block_size);
        /**
         * Number of tag bits is given by the address size
         * minus the number of offset bits
         * 
         * e.g. address = 64 bit, block size = 64 bytes
         * -> number of tag bits = 64 - log(64) = 64 - 6 = 58
         */
        cache->n_tagbits = 64 - cache->n_offbits;
        cache->cache_lines = calloc(cache->n_cache_lines, 
                                    sizeof(cache_line_t));
        return cache;
}

/** 
 * cache_destroy(): free allocated cache resources
 * @cache: cache object to be deallocated
 */
void cache_destroy(cache_t *cache)
{
        free(cache->cache_lines);
        free(cache);
}

/**
 * cache_lookup(): lookup the generated address in the cache
 * @cache: the cache to search
 * @addr: 64 bit address generate by a read or write
 * @rw: 'r' on a load or 'w' on a store
 * return: 1 if found, 0 otherwise
 */
int cache_lookup(cache_t *cache, u64 addr, char rw)
{
        u8 hit;
        cache_line_t *line;

        for (hit = 0, line = cache->cache_lines; 
             line < cache->cache_lines + cache->n_cache_lines; line++) {
                if (line->tag == (addr >> cache->n_offbits) && line->v) {
                        hit = 1;
                        line->u = 1;
                        if (rw == 'w')
                                line->m = 1;
                        break;
                }
        }
        if (hit)
                printf("Cache Hit: %s to %lx\n", 
                        (rw == 'r') ? "Read" : "Write", addr);
        return hit;
}

/**
 * cache_evict_replace(): find cache line to evict and replace with new block
 * @cache: cache to evict a line from
 * @addr: address to generated a cache miss
 * @rw: read or write
 */
void cache_evict_replace(cache_t *cache, u64 addr, char rw)
{
        printf("Cache Miss: %s to %lx, ", 
                rw == 'r' ? "Read" : "Write", addr);

        int i;
        cache_line_t *line = NULL;
        
        for (i = 0;; i = (i + 1) % cache->n_cache_lines) {
                line = &(cache->cache_lines[i]);
                /**
                 * If the cache line has either an unset use bit/reference 
                 * bit, or the cache line is marked invalid, it is chosen
                 * for eviction.
                 *
                 * For valid cache lines with with a set use bit, unset
                 * the use bit during this eviction cycle
                 */
                if (line->v && line->u)
                        line->u = 0;
                else {
                        printf("Eviciting line at %lx, ",
                                (u64)(line->tag << cache->n_offbits));
                        if (line->m)
                                printf("writing back...\n");
                        else
                                printf("no write back\n");
                        break;
                }
        }
        /* Replace cache block with new address and set other bits */
        line->tag = addr >> cache->n_offbits;
        line->v = 1;
        line->u = 1;
        if (rw == 'w')
                line->m = 1;
}

/**
 * cache_simulate(): simulate cache performance using a binary instrumented
 *                   with llvm to print the addresses of loads and stores
 *                   that prints to stdin
 * @cache: cache structure to perform the simulation with
 * return: 0 if simulation executes successfully, -1 otherwise
 */
int cache_simulate(cache_t *cache)
{
        char rw;   // read or write
        u64  addr; // address read or written to
        
        /* Track the number of cache hits and misses */
        int n_hits = 0, n_misses = 0;

        while (!feof(stdin)) {
                if (scanf("%c%lx\n", &rw, &addr) < 2) {
                        fprintf(stderr, "scanf failed\n");
                        return -1;
                }
                assert(rw == 'r' || rw == 'w');
                if (!cache_lookup(cache, addr, rw)) {
                        cache_evict_replace(cache, addr, rw);
                        n_misses++;
                } else
                        n_hits++;
        }
        putchar('\n');
        printf("Number of Cache Hits:\t%d\n"
               "Number of Cache Misses:\t%d\n"
               "Total Loads and Stores:\t%d\n"
               "Percent Cache Hits:\t%f\n",
               n_hits, n_misses, n_hits + n_misses,
               ((float)(n_hits) / (float)(n_hits + n_misses)));
        return 0;
}

int main(int argc, char *argv[])
{
        if (argc < 5) {
            fprintf(stderr, "Usage: ./cache-simulator "
                            "--data-block-size [NUMBER] "
                            "--cache-size [NUMBER]\n");
            exit(1);
        }

        u8 cache_line_size = 0;
        u16 cache_size     = 0;

        for (int i = 1; i < argc; i++) {
                if (!strcmp(argv[i], "--data-block-size"))
                        cache_line_size = atoi(argv[++i]);
                else if (!strcmp(argv[i], "--cache-size"))
                        cache_size = atoi(argv[++i]);
        }
        assert(!(cache_line_size == 0 || cache_size == 0));

        cache_t *cache = cache_create(cache_size, cache_line_size);
        if (cache_simulate(cache) < 0) {
                fprintf(stderr, "Simulation failure. Exiting...\n");
                cache_destroy(cache);
                exit(1);
        }
        cache_destroy(cache);
        exit(0);
}
