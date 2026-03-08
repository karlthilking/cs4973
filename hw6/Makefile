CFLAGS = -g3 -O0
LLVMFLAGS = -emit-llvm -S
OPTFLAGS = -load-pass-plugin=$$PWD/lib-emit-addr.so -passes="emit-addr-pass" -disable-output

BLOCK_SIZE = 128
CACHE_SIZE = 4096

build: cache-simulator lib-emit-addr.so 01-scan.ll 02-matmul.ll 03-quicksort.ll

# shared library for instrumentation pass
lib-emit-addr.so: emit-addr-pass.cpp
	clang++ -shared -fPIC -o $@ $<

cache-simulator: cache-simulator.c
	gcc $(CFLAGS) -o $@ $<

# LLVM-IR files
%.ll: %.c
	clang $(LLVMFLAGS) -S $<

check: build
	opt $(OPTFLAGS) 02-matmul.ll 2>&1 1>/dev/null | ./cache-simulator --data-block-size $(BLOCK_SIZE) --cache-size $(CACHE_SIZE)

# %-llvm: %.ll
# 	clang -o $@ $<

clean:
	rm -f cache-simulator *-llvm *.ll *.so
