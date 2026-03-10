CFLAGS = -g3 -O0
LLVMFLAGS = -emit-llvm -S
OPTFLAGS = -load-pass-plugin=$$PWD/lib-emit-addr.so -passes="emit-addr-pass"

BLOCK_SIZE = 128
CACHE_SIZE = 4096

build: cache-simulator lib-emit-addr.so 01-scan-llvm 02-matmul-llvm 03-quicksort-llvm 04-random-llvm

# shared library for instrumentation pass
lib-emit-addr.so: emit-addr-pass.cpp
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
	./02-matmul-llvm | ./cache-simulator --data-block-size $(BLOCK_SIZE) --cache-size $(CACHE_SIZE)
clean:
	rm -f cache-simulator *-llvm *.ll *.so
