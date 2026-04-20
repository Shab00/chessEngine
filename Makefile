CC := gcc
CFLAGS := -Iinclude -std=c11 -Wall -Wextra -O2

SRCDIR := src
TESTDIR := tests
BUILDDIR := build
OBJDIR := $(BUILDDIR)/objs

SRCS := $(wildcard $(SRCDIR)/*.c)
SRCS_NO_MAIN := $(filter-out $(SRCDIR)/main.c,$(SRCS))

OBJS := $(patsubst $(SRCDIR)/%.c,$(OBJDIR)/%.o,$(SRCS_NO_MAIN))
TEST_SRCS := $(wildcard $(TESTDIR)/*.c)
TEST_BINS := $(patsubst $(TESTDIR)/%.c,$(BUILDDIR)/%,$(TEST_SRCS))

ENGINE := $(BUILDDIR)/engine

.PHONY: all engine tests run-tests clean help sanitize

all: $(TEST_BINS)

engine: $(ENGINE)

$(ENGINE): $(SRCDIR)/main.c $(OBJS) | $(BUILDDIR)
	$(CC) $(CFLAGS) $(OBJS) $< -o $@

$(BUILDDIR):
	mkdir -p $(BUILDDIR)

$(OBJDIR):
	mkdir -p $(OBJDIR)

$(OBJDIR)/%.o: $(SRCDIR)/%.c | $(OBJDIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILDDIR)/%: $(TESTDIR)/%.c $(OBJS) | $(BUILDDIR)
	$(CC) $(CFLAGS) $(OBJS) $< -o $@

tests: all

run-tests: tests
	./build/perft_runner runfile

clean:
	rm -rf $(BUILDDIR)

help:
	@echo "Usage:"
	@echo "  make            # build all test binaries into build/"
	@echo "  make engine     # build UCI engine binary as build/engine"
	@echo "  make run-tests  # build and run ./build/perft_runner runfile"
	@echo "  make clean      # remove build/"

sanitize:
	$(MAKE) CFLAGS='-Iinclude -std=c11 -g -O0 -fsanitize=address,undefined -fno-omit-frame-pointer -Wall -Wextra' all
