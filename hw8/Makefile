CC=gcc
CFLAGS=-g3 -O0 -lpthread

build: rw-reader-pref rw-writer-pref

%: %.c
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -f rw-reader-pref rw-writer-pref

