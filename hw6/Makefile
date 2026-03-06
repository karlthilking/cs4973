CC = gcc
CFLAGS = -g3 -O0

build: cache-simulator

cache-simulator: cache-simulator.c
	$(CC) $(CFLAGS) -o $@ $<

%-llvm: %.c
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -f cache-simulator *-llvm *.bc
