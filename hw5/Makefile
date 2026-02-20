CC = gcc
CFLAGS = -g3 -O0

all: mysh myls inode

mysh: mysh.c
	$(CC) $(CFLAGS) -o $@ $<

myls: myls.c
	$(CC) $(CFLAGS) -o $@ $<

inode: inode.c
	$(CC) $(CFLAGS) -o $@ $<

check:
	diff xv6-sh.c mysh.c | grep '^>' | wc -l
	diff xv6-ls.c myls.c | grep '^>' | wc -l

clean:
	rm -f mysh myls inode
