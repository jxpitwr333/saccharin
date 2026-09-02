CC ?= gcc
CFLAGS ?= -std=c99 -Wall -Wextra -DUNITY_BUILD

run:
	$(CC) $(CFLAGS) main.c -o compiler
	./compiler

clean:
	rm -f compiler

.PHONY: clean
