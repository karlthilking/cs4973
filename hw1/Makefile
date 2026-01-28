CC = gcc
CFLAGS = -g3 -O3

build: count matrix ckpt readckpt libckpt.so

count: count.c
	$(CC) $(CFLAGS) -rdynamic -o $@ $<

matrix: matrix.c
	$(CC) $(CFLAGS) -rdynamic -o $@ $<

ckpt: ckpt.c
	$(CC) $(CFLAGS) -rdynamic -o $@ $<

readckpt: readckpt.c
	$(CC) $(CFLAGS) -rdynamic -o $@ $<

libckpt.so: libckpt.c
	$(CC) $(CFLAGS) -shared -fPIC -o $@ $<

check: build
	./ckpt ./matrix 50000 8 & sleep 8
	pkill -12 --newest matrix
	./readckpt ckpt.dat

clean:
	rm -f count matrix ckpt readckpt libckpt.so ckpt.dat
    
    
