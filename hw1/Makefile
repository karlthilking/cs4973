CC = gcc
CFLAGS = -g3 -O0

build: count ckpt readckpt libckpt.so restart

count: count.c
	$(CC) $(CFLAGS) -rdynamic -o $@ $<

ckpt: ckpt.c
	$(CC) $(CFLAGS) -rdynamic -o $@ $<

readckpt: readckpt.c
	$(CC) $(CFLAGS) -rdynamic -o $@ $<

libckpt.so: libckpt.c
	$(CC) $(CFLAGS) -shared -fPIC -o $@ $<

restart: restart.c 
	$(CC) $(CFLAGS) -static \
		-Wl,-Ttext-segment=5000000 \
		-Wl,-Tdata=5100000 \
		-Wl,-Tbss=5200000 -o $@ $< 

check:
	./ckpt ./count & sleep 10
	pkill -12 --newest count
	./restart ckpt.dat & sleep 10
	pkill -12 --newest restart

clean:
	rm -f count ckpt readckpt libckpt.so ckpt.dat
    
    
    
