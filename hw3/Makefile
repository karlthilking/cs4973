CC = gcc
CFLAGS = -fsanitize=address -fsanitize=leak

define build 
	$(CC) $(CFLAGS) -o shell shell.c
	@echo '$(CC) $(CFLAGS) -o shell shell.c'
endef

debug:
	$(eval CFLAGS += -Wall -Wextra -Werror -g3 -O0)
	@$(call build)

release:
	$(eval CFLAGS := -O3)
	@$(call build)

build:
	$(eval CFLAGS := -g3 -O0)
	@$(call build)

check:
	@$(call build)
	./shell

clean:
	rm -f shell


