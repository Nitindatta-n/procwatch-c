CC ?= cc
CFLAGS ?= -std=c11 -O2 -Wall -Wextra -Wpedantic -Werror

procwatch: procwatch.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $< $(LDFLAGS) -o $@

.PHONY: test clean
test: procwatch
	python3 test.py
	./procwatch --once

clean:
	$(RM) procwatch
