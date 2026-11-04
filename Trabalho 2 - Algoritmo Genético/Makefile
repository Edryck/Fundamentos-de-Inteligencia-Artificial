CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -Werror -O2 -D_POSIX_C_SOURCE=200809L
LDLIBS = -lm

SRC = $(wildcard src/*.c)
LIB_SRC = $(filter-out src/main.c,$(SRC))
TEST_SRC = $(wildcard tests/*.c)
HAS_MAIN = $(wildcard src/main.c)

ifeq ($(HAS_MAIN),)
all:
	@echo "src/main.c not found"; exit 1
else
all: trabalho2
endif

trabalho2: $(SRC) $(wildcard src/*.h)
	$(CC) $(CFLAGS) -o $@ $(SRC) $(LDLIBS)

tests/run_tests: $(LIB_SRC) $(TEST_SRC) $(wildcard src/*.h) $(wildcard tests/*.h)
	$(CC) $(CFLAGS) -o $@ $(LIB_SRC) $(TEST_SRC) $(LDLIBS)

test: tests/run_tests
	./tests/run_tests

clean:
	rm -f trabalho2 tests/run_tests

.PHONY: all test clean
