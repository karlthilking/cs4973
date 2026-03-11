CFLAGS = -g3 -O0
LLVMFLAGS = -emit-llvm -S
OPTFLAGS = -load-pass-plugin=$$PWD/libemitglobal.so -passes="emit-addr-pass"

BLOCK_SIZE = 128         
CACHE_SIZE = 4096        
MODE = summary           
PROG = 03-quicksort-llvm 
ARGS = --data-block-size $(BLOCK_SIZE) --cache-size $(CACHE_SIZE) --mode $(MODE)

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
        
	for (( i=1; i<=32; i*=2 )) ; do \
		if [[ $$i -eq 1 ]]; then \
			echo "Direct Mapped"; \
		elif [ $$i -eq 32 ]]; then \
			echo "Fully Associative"; \
		else \
			echo "$$i-Way Associative"; \
		fi \
		./$(PROG) | ./cache-simulator $(ARGS) --associativity $$i; \
	done
        
clean:
	rm -f cache-simulator *-llvm *.ll *.so
