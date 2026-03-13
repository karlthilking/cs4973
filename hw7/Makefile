CC=gcc
CFLAGS=-g3 -O0 -lpthread

build: 2-philosophers-mutex 3-philosophers-sem

%: %.c
	$(CC) $(CFLAGS) -o $@ $<

check: build
	./2-philosophers-mutex --num-threads 4
	./3-philosophers-sem --num-threads 4

clean:
	rm -f 2-philosophers-mutex 3-philosophers-sem
