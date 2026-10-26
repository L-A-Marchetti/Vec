CC        ?= cc
AR        ?= ar
CFLAGS    ?= -std=c11 -O2 -Wall -Wextra -Werror -pedantic
CPPFLAGS  ?=
LDFLAGS   ?=
PREFIX    ?= /usr/local
DESTDIR   ?=
BUILD_DIR ?= build

LIB := $(BUILD_DIR)/libvec.a
TEST_BIN := $(BUILD_DIR)/test_vec
FAIL_BIN := $(BUILD_DIR)/test_alloc_failure
EXAMPLE_BIN := $(BUILD_DIR)/example
BENCH_BIN := $(BUILD_DIR)/bench_vec
BENCH_ARGS ?= 1000000 7

.PHONY: all test test-example example bench sanitize clean install
all: $(LIB)

$(BUILD_DIR):
	mkdir -p $@

$(BUILD_DIR)/vec.o: vec.c vec.h | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c vec.c -o $@

$(LIB): $(BUILD_DIR)/vec.o
	$(AR) rcs $@ $^

$(TEST_BIN): tests/test_vec.c tests/test.h vec.h $(LIB)
	$(CC) $(CPPFLAGS) $(CFLAGS) -I. tests/test_vec.c $(LIB) $(LDFLAGS) -o $@

$(FAIL_BIN): tests/test_alloc_failure.c tests/test.h vec.c vec.h | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -I. tests/test_alloc_failure.c $(LDFLAGS) -o $@

test: $(TEST_BIN) $(FAIL_BIN)
	$(TEST_BIN)
	$(FAIL_BIN)

$(EXAMPLE_BIN): example.c vec.h $(LIB)
	$(CC) $(CPPFLAGS) $(CFLAGS) -I. example.c $(LIB) $(LDFLAGS) -o $@

example: $(EXAMPLE_BIN)

test-example: $(EXAMPLE_BIN) tests/test_example.sh
	sh tests/test_example.sh $(EXAMPLE_BIN)

$(BENCH_BIN): benchmarks/bench_vec.c vec.h $(LIB)
	$(CC) $(CPPFLAGS) $(CFLAGS) -I. benchmarks/bench_vec.c $(LIB) $(LDFLAGS) -o $@

bench: $(BENCH_BIN)
	$(BENCH_BIN) $(BENCH_ARGS)

sanitize:
	$(MAKE) BUILD_DIR=$(BUILD_DIR)/sanitize CFLAGS='-std=c11 -O1 -g -Wall -Wextra -Werror -pedantic -fsanitize=address,undefined -fno-omit-frame-pointer' LDFLAGS='-fsanitize=address,undefined' test test-example

clean:
	rm -rf $(BUILD_DIR)

install: $(LIB) vec.h
	install -d $(DESTDIR)$(PREFIX)/include $(DESTDIR)$(PREFIX)/lib
	install -m 644 vec.h $(DESTDIR)$(PREFIX)/include/vec.h
	install -m 644 $(LIB) $(DESTDIR)$(PREFIX)/lib/libvec.a
