CC ?= gcc
CFLAGS ?= -std=c99 -Wall -Wextra -DUNITY_BUILD

all: main.c
	$(CC) $(CFLAGS) main.c -o compiler

clean:
	rm -f compiler

.PHONY: clean
