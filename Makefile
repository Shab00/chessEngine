CC := gcc
CFLAGS := -Iinclude -std=c11 -Wall -Wextra -O2

SRCDIR := src
TESTDIR := tests
BUILDDIR := build
OBJDIR := $(BUILDDIR)/objs

SRCS := $(wildcard $(SRCDIR)/*.c)
# avoid linking src/main.c (it defines its own main)
SRCS_NO_MAIN := $(filter-out $(SRCDIR)/main.c,$(SRCS))

OBJS := $(patsubst $(SRCDIR)/%.c,$(OBJDIR)/%.o,$(SRCS_NO_MAIN))
TEST_SRCS := $(wildcard $(TESTDIR)/*.c)
TEST_BINS := $(patsubst $(TESTDIR)/%.c,$(BUILDDIR)/%,$(TEST_SRCS))

.PHONY: all tests run-tests clean help sanitize

all: $(TEST_BINS)

# Ensure build dirs exist
$(BUILDDIR):
	mkdir -p $(BUILDDIR)

$(OBJDIR):
	mkdir -p $(OBJDIR)

# Compile source objects (excluding any file that defines its own main)
$(OBJDIR)/%.o: $(SRCDIR)/%.c | $(OBJDIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Link each test program against the collected objects
# This avoids linking src/main.c which could conflict with tests' mains.
$(BUILDDIR)/%: $(TESTDIR)/%.c $(OBJS) | $(BUILDDIR)
	$(CC) $(CFLAGS) $(OBJS) $< -o $@

tests: all

run-tests: tests
	# Run the verified perft runner against tests/perft_tests.txt
	./build/perft_runner runfile

clean:
	rm -rf $(BUILDDIR)

help:
	@echo "Usage:"
	@echo "  make            # build all test binaries into build/"
	@echo "  make run-tests  # build and run ./build/perft_runner runfile"
	@echo "  make clean      # remove build/"

sanitize:
	$(MAKE) CFLAGS='-Iinclude -std=c11 -g -O0 -fsanitize=address,undefined -fno-omit-frame-pointer -Wall -Wextra' all
