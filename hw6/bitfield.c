#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <limits.h>
#include <stdint.h>

typedef struct cache_line_hdr {
    unsigned long   tag : 58;
    unsigned int    m   : 1;
    unsigned int    v   : 1;
    unsigned int    u   : 1;
    unsigned char   pad : 3; 
} cache_line_hdr_t;

typedef struct cache_line {
    cache_line_hdr_t    hdr;
} cache_line_t;

int
log2i(int x)
{
    int i;
    for (i = 0; x > 1; ++i, x >>= 1)
        ;
    return i;
}

int
main(void)
{
    const uint16_t  CACHE_SIZE      = 32 * 1024;
    const uint8_t   CACHE_LINE_SIZE = 128;
    const uint16_t  N_CACHE_LINES   = CACHE_SIZE / CACHE_LINE_SIZE;

    assert(N_CACHE_LINES == 256);

    const uint8_t N_OFFBITS = log2i(CACHE_LINE_SIZE);
    const uint8_t N_TAGBITS = 64 - N_OFFBITS;
    
    assert(N_OFFBITS == 7);
    assert(N_TAGBITS == 57);

    const uint64_t OFFBITMASK = (1 << N_OFFBITS) - 1;
    const uint64_t TAGBITMASK = ~OFFBITMASK;

    assert(OFFBITMASK == 127);
    assert(TAGBITMASK == ULONG_MAX - OFFBITMASK);

    exit(0);
}
