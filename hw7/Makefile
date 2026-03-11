CC=gcc
CFLAGS=-g3 -O0 -lpthread

build: 2-philosophers-mutex 3-philosophers-sem

%: %.c
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -f 2-philosophers-mutex 3-philosophers-sem
