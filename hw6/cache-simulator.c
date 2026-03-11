/* cache-simulator.c */
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <assert.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#define MAGIC (((1 << 13) % 47) - 5)

/* integer typedefs */
typedef int8_t   i8;
typedef int16_t  i16;
typedef int32_t  i32;
typedef int64_t  i64;
typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

/* Event that modified the cache state */
enum { C_HIT, C_MISS, C_EVICT, C_INV };

typedef struct cache_line {
        /**
         * Cache line layout:
         *  tag | index | m | v | u
         * tag: upper 64 - # index bits - # offset bits - 3
         * m: modified bit
         * v: valid bit
         * u: use bit
         */
         u64 tag   : 61; // tag bits
         u8  m     : 1;  // modified bit
         u8  v     : 1;  // valid bit
         u8  u     : 1;  // use bit
} cache_line_t;

typedef struct cache_set {
        cache_line_t    *cache_lines; // cache lines in this set
        union {
                u16 index;      // index of set
                u16 empty;      // 1 if set has no valid lines
        };
} cache_set_t;

typedef struct cache {
        /**
         * Any type of cache can be interpreted as containing sets
         * - Fully associative: 1 set of |cache blocks| entries
         * - Set associative: |cache blocks|/|associativity| sets of 
         *                    |associativity| entries
         * - Direct mapped: |cache blocks| sets of 1 entry
         */
        cache_set_t     *cache_sets;    // sets in this cache
        u16             nr_sets;        // number of sets
        u8              nr_set_entries; // number of entries per set
        u8              nr_offbits;     // number of offset bits
        u8              nr_ixbits;      // number of index bits
} cache_t;

/**
 * log2i(): compute log base 2 of an integer, used to calculate cache
 *          parameters
 * @x: the value to compute the base 2 logarithm of
 */
u8 log2i(int x)
{
        u8 i = 0;
        for (; x > 1; x >>= 1, i++)
                ;
        return i;
}

/**
 * cache_create(): Creates a cache structure with parameters corresponding to
 *                 a given cache size, block size, and associativity
 * @cache_size: total size of the cache in bytes
 * @data_block_size: size of each data block/cache line in bytes
 * @associativity: associativity of the cache (n == 1 -> direct mapped,
 *                 1 < n < number of cache lines -> n-way set associative,
 *                 n == number of cache lines -> fully associative
 */
cache_t *cache_create(u16 cache_size, u8 data_block_size, u8 associativity)
{
        /**
         * Calculated parameters:
         * - number of cache lines = cache size / data block size
         * - number of sets = number of cache lines / associativity
         * - number of entries per set = associativity
         * - number of bits in address for offset = log2(data block size)
         * - number of bits in address for index = log2(number of sets)
         */
        cache_t *cache = malloc(sizeof(*cache));
        u16 nr_cache_lines = cache_size / data_block_size;
        cache->nr_sets = nr_cache_lines / associativity;
        cache->nr_set_entries = associativity;
        cache->nr_offbits = log2i(data_block_size);
        cache->nr_ixbits = log2i(cache->nr_sets);
        
        /**
         * Allocate nr_sets sets of cache lines, initialize their set of cache
         * lines, and set the empty to be true
         *
         * Note: might be better to make empty a magic number to avoid 
         * possibility of collision
         */
        cache->cache_sets = malloc(sizeof(cache_set_t) * cache->nr_sets);
        for (cache_set_t *set = cache->cache_sets; 
             set < cache->cache_sets + cache->nr_sets; set++) {
                set->empty = MAGIC;
                set->cache_lines = calloc(cache->nr_set_entries, 
                                          sizeof(cache_line_t));
        }
        return cache;
}

/**
 * cache_destroy(): Deallocates the resources of the associated cache
 * @cache: cache to free resources from
 */
void cache_destroy(cache_t *cache)
{
        for (cache_set_t *set = cache->cache_sets;
             set < cache->cache_sets + cache->nr_sets; set++)
                free(set->cache_lines);
        free(cache->cache_sets);
        free(cache);
}

/**
 * cache_get_index(): Extracts the index bits from an address
 * @cache: cache providing parameters to determine which bits used for index
 * @addr: address generated from a read or write
 */
u64 cache_get_index(cache_t *cache, u64 addr)
{
        u64 mask = ((1 << cache->nr_ixbits) - 1) << cache->nr_offbits;
        return (addr & mask) >> cache->nr_offbits;
}

void cache_report(u64 addr, char rw, u8 event)
{
        switch (event) {
        case C_HIT:
                printf("Cache Hit: %s to %lx\n", 
                        (rw == 'r') ? "Read" : "Write", addr);
                break;
        case C_MISS:
                printf("Cache Miss: %s to %lx\n",
                        (rw == 'r') ? "Read" : "Write", addr);
                break;
        case C_EVICT:
                printf("Cache Eviction: Evicted %lx\n", addr);
                break;
        case C_INV:
                printf("Cache Replacement: Replaced invalid line\n");
                break;
        }
}

/**
 * cache_set_search(): Indexes into set corresponding to the index bits of
 *                     generated address. If the cache is fully associative,
 *                     returns the one set which represents the entire cache.
 *                     Additionally, if a set with the matching index is not
 *                     allocated, return NULL.
 * @cache: cache to index into
 * @addr: addr to extract index bits from
 */
cache_set_t *cache_set_search(cache_t *cache, u64 addr)
{
        /* If fully associative, there is only one set to search */
        if (cache->nr_sets == 1)
                return cache->cache_sets;
        
        /* If direct mapped or set associative, lookup index match */
        for (cache_set_t *set = cache->cache_sets;
             set < cache->cache_sets + cache->nr_sets; set++) {
                if (set->empty == MAGIC)
                        continue;
                else if (set->index == cache_get_index(cache, addr))
                        return set;
        }
        return NULL;
}

/**
 * cache_line_search(): search cache set in cache for a matching cache line,
 *                      if the cache is fully associative, the 'set' is the
 *                      entire cache
 * @cache: cache being searched
 * @set: set in the cache that was indexed into and being searched
 * @addr: address generated by read or write
 * @rw: if the address was generated by a read or write
 */
int cache_line_search(cache_t *cache, cache_set_t *set, u64 addr, char rw)
{       
        /* Extract tag from the given address */
        u64 tag = addr >> (cache->nr_ixbits + cache->nr_offbits);
        
        /* If direct mapped, only one comparison */
        if (cache->nr_set_entries == 1) {
                cache_line_t *line = set->cache_lines;
                if (line->v && line->tag == tag) {
                        cache_report(addr, rw, C_HIT);
                        return 1;
                }
                cache_report(addr, rw, C_MISS);
                return 0;
        }
        
        /* If set associative or fully associative, search entire set space */
        cache_line_t *line, *end = set->cache_lines + cache->nr_set_entries;
        for (line = set->cache_lines; line < end; line++) {
                if (line->v && line->tag == tag) {
                        cache_report(addr, rw, C_HIT);
                        return 1;
                }
        }
        cache_report(addr, rw, C_MISS);
        return 0;
}

/**
 * cache_replace_in_line(): Replace a line in the specified set with LRU
 *                       approximation
 * @cache: cache which given set resides in
 * @set: set to evict a line from
 * @addr: address to replace with evicted line
 * @rw: whether the cache miss originated from a read or a write
 */
int cache_replace_line(cache_t *cache, cache_set_t *set, u64 addr, char rw)
{
        u64 tag = addr >> (cache->nr_ixbits + cache->nr_offbits);
        while (1) {
                for (cache_line_t *line = set->cache_lines;
                     line < set->cache_lines + cache->nr_set_entries; line++) {
                        if (line->v && line->u) {
                                line->u = 0;
                                continue;
                        } else if (!(line->v)) {
                                line->tag = tag;
                                line->v = 1;
                                line->u = 1;
                                if (rw == 'w')
                                        line->m = 1;
                                cache_report(0, 0, C_INV);
                                return 1;
                        } else if (!(line->u)) {
                                u64 oldtag = line->tag << cache->nr_offbits;
                                line->tag = tag;
                                line->v = 1;
                                line->u = 1;
                                if (rw == 'w')
                                        line->m = 1;
                                cache_report(oldtag, 0, C_EVICT);
                                return 1;
                        }
                }
        }
        return 0;
}

int cache_replace_set(cache_t *cache, u64 addr, char rw)
{
        for (cache_set_t *set = cache->cache_sets;
             set < cache->cache_sets + cache->nr_sets; set++) {
                if (set->empty == MAGIC) {
                        set->index = cache_get_index(cache, addr);
                        return cache_replace_line(cache, set, addr, rw);
                }
        }
        return 0;
}

/*
 * cache_simulate(): Simulates cache with reads and writes reported from
 *                   binary instrumented with llvm module pass
 * @cache: cache to use for simulation
 */
int cache_simulate(cache_t *cache, char mode)
{       
        /* Disable stdout during simulation if in summary mode */
        int fd;
        if (mode == 's') {
                fd = open("/dev/stdout", O_WRONLY);
                if (fd < 0) {
                        perror("open");
                        return -1;
                }
                close(STDOUT_FILENO);
        }

        int nr_hits = 0, nr_misses = 0; // number of cache hits and misses
        u64  addr;                      // address of read or write
        char rw;                        // if access was a read or write
        
        while (!feof(stdin)) {
                if (scanf("%c%lx\n", &rw, &addr) < 2) {
                        fprintf(stderr, "Failed to retrieve load/store info");
                        return -1;
                }
                cache_set_t *set;
                if ((set = cache_set_search(cache, addr)) != NULL &&
                    cache_line_search(cache, set, addr, rw)) {
                        nr_hits++;
                        continue;
                } else if (set != NULL) {
                        nr_misses++;
                        int rc = cache_replace_line(cache, set, addr, rw);
                        if (!rc) {
                                fprintf(stderr, "Cache evicition failed\n");
                                return -1;
                        }
                } else {
                        nr_misses++;
                        int rc = cache_replace_set(cache, addr, rw);
                        if (!rc) {
                                fprintf(stderr, "Cache eviction failed\n");
                                return -1;
                        }
                }
        }
        
        if (mode == 's') {
                char buf[512];
                snprintf(buf, sizeof(buf),
                         "Number of total memory accesses:\t%d\n"
                         "Number of cache hits:\t\t\t%d\n"
                         "Number of cache misses:\t\t\t%d\n"
                         "Percent of cache hits:\t\t\t%f\n",
                         nr_hits + nr_misses, nr_hits, nr_misses,
                         (float)nr_hits / (float)(nr_hits + nr_misses));
                write(fd, buf, strlen(buf));
        }
        return 0;
}

int main(int argc, char *argv[])
{
        if (argc < 7) {
                fprintf(stderr, "Usage: ./cache-simulator "
                                "--data-block-size [N] "
                                "--cache-size [N] "
                                "--mode [verbose | summary] "
                                "--associativity [N] (optional)\n");
                exit(1);
        }
        
        u8 data_block_size = 0;
        u16 cache_size = 0;
        u8 associativity = 0;
        char mode = 0;
        
        for (int i = 1; i < argc; i += 2) {
                if (!strcmp("--data-block-size", argv[i]))
                        data_block_size = atoi(argv[i + 1]);
                else if (!strcmp("--cache-size", argv[i]))
                        cache_size = atoi(argv[i + 1]);
                else if (!strcmp("--mode", argv[i]))
                        mode = (strcmp(argv[i + 1], "verbose")) ? 's' : 'v';
                else if (!strcmp("--asociativity", argv[i]))
                        associativity = atoi(argv[i + 1]);
        }
        
        if (!(data_block_size && cache_size && mode)) {
                fprintf(stderr, "A data block size, cache size or mode "
                                "were not selected\n");
                exit(1);
        } else if (associativity == 0) {
                /* Default to fully associative cache */
                associativity = cache_size / data_block_size;
        }

        cache_t *cache = cache_create(cache_size, data_block_size, 
                                      associativity);
        if (cache_simulate(cache, mode) < 0) {
                fprintf(stderr, "cache-simulation failed. Exiting...\n");
                cache_destroy(cache);
                exit(1);
        }

        cache_destroy(cache);
        exit(0);
}
