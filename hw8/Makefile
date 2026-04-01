CC=gcc
CFLAGS=-g3 -O0 -lpthread

build: rw-reader-pref rw-writer-pref

%: %.c
	$(CC) $(CFLAGS) -o $@ $<

check: build
	@echo 'Executing rw-reader-pref' && sleep 1
	./rw-reader-pref
	@sleep 2
	@echo 'Executing rw-writer-pref' && sleep 1
	./rw-writer-pref

clean:
	rm -f rw-reader-pref rw-writer-pref

