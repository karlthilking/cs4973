CC = gcc
CFLAGS = -g3 -O0

build: shell

shell: shell.c
	$(CC) $(CFLAGS) -o $@ $<

check: build
	./shell

clean:
	rm -f shell

release: shell.c
	$(eval CFLAGS := -O3)
	$(CC) $(CFLAGS) -o $@ $<

