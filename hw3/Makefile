CC = gcc
CFLAGS = -g3 -O0

build: myshell

myshell: myshell.c
	$(CC) $(CFLAGS) -o $@ $<

run: build
	./myshell

check: build
	./myshell < input.txt

clean:
	rm -f myshell
