BLOCK_SIZE=128
CACHE_SIZE=4096

cache-simulator: cache-simulator.c
	gcc -o $@ $<

%-llvm: %.c
	gcc -o $@ $<

clean:
	rm -f 

check: test-llvm cache-simulator
	./test-llvm 2>&1 1>/dev/null | cache-simulator  


