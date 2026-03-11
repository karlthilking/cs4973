CFLAGS = -g3 -O0
LLVMFLAGS = -emit-llvm -S
OPTFLAGS = -load-pass-plugin=$$PWD/libemitglobal.so -passes="emit-addr-pass"

BLOCK_SIZE = 128        # data block size parameter to cache simulator
CACHE_SIZE = 4096       # cache size parameter to cache simulator
MODE = summary          # mode parameter to cache simulator
ASSOC = 32              # associativity parameter to cache simulator
PROG = 02-matmul-llvm   # test program to pipe to cache simulator

build: cache-simulator libemitglobal.so 01-scan-llvm 02-matmul-llvm 03-quicksort-llvm 04-random-llvm

# shared library for instrumentation pass
libemitglobal.so: emit-global-addr.cpp
	clang++ -shared -fPIC -o $@ $<

cache-simulator: cache-simulator.c
	gcc $(CFLAGS) -o $@ $<

# LLVM-IR Files without pass
%.ll: %.c
	clang $(LLVMFLAGS) -o $@ $<

# compile into *-llvm files with pass
%-llvm: %.ll
	opt $(OPTFLAGS) $< -S -o opt-$<
	clang opt-$< -o $@

check: build
	./$(PROG) | ./cache-simulator --data-block-size $(BLOCK_SIZE) --cache-size $(CACHE_SIZE) --mode $(MODE) --associativity $(ASSOC)
clean:
	rm -f cache-simulator *-llvm *.ll *.so
