CFLAGS = -g3 -O0
LLVMFLAGS = -emit-llvm -S

build: cache-simulator 01_test_scan.ll 02_test_matmul.ll 03_test_quicksort.ll

cache-simulator: cache-simulator.c
	gcc $(CFLAGS) -o $@ $<

%.ll: %.c
	clang $(LLVMFLAGS) -S $<

clean:
	rm -f cache-simulator *-llvm *.ll
