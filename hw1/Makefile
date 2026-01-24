CC = gcc
CFLAGS = -g3 -O0

all: count ckpt readckpt libckpt.so

count: count.c
	$(CC) $(CFLAGS) -rdynamic -o $@ $<

ckpt: ckpt.c
	$(CC) $(CFLAGS) -rdynamic -o $@ $<

readckpt: readckpt.c
	$(CC) $(CFLAGS) -rdynamic -o $@ $<

libckpt.so: libckpt.c
	$(CC) $(CFLAGS) -shared -fPIC -o $@ $<

clean:
	rm -f ckpt readckpt libckpt.so
    
    
    
