CC=gcc
CFLAGS=-g3 -O0

BLOCK_SIZE=128
CACHE_SIZE=4096

build: matmul-llvm cache-simulator

cache-simulator: cache-simulator.c
	$(CC) $(CFLAGS) -o $@ $<

%-llvm: %.c
	$(CC) $(CFLAGS) -o $@ $<

check:
	./matmul 2>&1 1>/dev/null | ./cache-simulator \
	--data-block-size $(BLOCK_SIZE) --cache-size $(CACHE_SIZE)

clean:
	rm -f *-llvm cache-simulator
