CC=gcc
CFLAGS=-g3 -O0 -lpthread

build: testlib.so ckpt pth

%.so: %.c
	$(CC) $(CFLAGS) -shared -fPIC -ldl -o $@ $<

%: %.c
	$(CC) $(CFLAGS) -o $@ $<

check:
	./ckpt ./pth

clean:
	rm -f *.so *.dat ckpt pth

