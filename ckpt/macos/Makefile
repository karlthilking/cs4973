CC=clang
CFLAGS=-g3 -O0 # -arch arm64e

build: ctx foo

%: %.c
	$(CC) $(CFLAGS) -o $@ $<
	objdump -D $@ > $@.S

clean:
	rm -rf ctx foo *.dSYM *.S
