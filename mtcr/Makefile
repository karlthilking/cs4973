CC = gcc
CFLAGS = -g3 -O0

build: libckpt.so ckpt open

%: %.c
	$(CC) $(CFLAGS) -o $@ $<

%.so: %.c
	$(CC) $(CFLAGS) -shared -fPIC -o $@ $<

clean:
	rm -f libckpt.so ckpt 0_fd_test
