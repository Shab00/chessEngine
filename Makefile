CC ?= gcc

# Directories
SRCDIR := src
INCDIR := include
LIBDIR := lib
BINDIR := bin
OBJDIR := build

# Names
LIB_NAME := libmylib.a
LIB_FILE := $(LIBDIR)/$(LIB_NAME)
EXEC_NAME := myprogram
TARGET := $(BINDIR)/$(EXEC_NAME)

# Sources / objects
SOURCES := $(wildcard $(SRCDIR)/*.c)
OBJECTS := $(patsubst $(SRCDIR)/%.c,$(OBJDIR)/%.o,$(SOURCES))

# Compiler flags
CPPFLAGS := -I$(INCDIR)
CFLAGS ?= -std=c11 -Wall -Wextra -g -O0
LDFLAGS ?=

# Optionally enable sanitizers:
SANITIZE ?= 0
ifeq ($(SANITIZE),1)
CFLAGS += -fsanitize=address,undefined
LDFLAGS += -fsanitize=address,undefined
endif

.PHONY: all debug release clean run perft help dirs

all: debug

debug: CFLAGS += -g -O0
debug: dirs $(TARGET)

release: CFLAGS := -std=c11 -O2 -DNDEBUG -Wall -Wextra
release: dirs $(TARGET)

# Exclude main.o from the static lib archive
LIB_OBJECTS := $(filter-out $(OBJDIR)/main.o,$(OBJECTS))

# Link final executable (link main.o with the static library)
$(TARGET): $(LIB_FILE) $(OBJDIR)/main.o | $(BINDIR)
	@echo "Linking $@"
	$(CC) $(CFLAGS) $(CPPFLAGS) -o $@ $(OBJDIR)/main.o -L$(LIBDIR) -lmylib $(LDFLAGS)

# Create static library from library objects
$(LIB_FILE): $(LIB_OBJECTS) | $(LIBDIR)
	@echo "Archiving $@"
	ar rcs $@ $(LIB_OBJECTS)

# Compile .c -> .o into OBJDIR
$(OBJDIR)/%.o: $(SRCDIR)/%.c | $(OBJDIR)
	@echo "Compiling $<"
	$(CC) $(CFLAGS) $(CPPFLAGS) -c $< -o $@

# Ensure directories exist
dirs: $(BINDIR) $(OBJDIR) $(LIBDIR)

$(BINDIR) $(OBJDIR) $(LIBDIR):
	@mkdir -p $@

# Run the program
run: debug
	@echo "Running $(TARGET) $(ARGS)"
	$(TARGET) $(ARGS)

# Perft convenience (your program should implement a --perft / similar)
perft: debug
	@echo "Running perft: $(TARGET) $(PERFT_ARGS)"
	$(TARGET) $(PERFT_ARGS)

clean:
	@echo "Cleaning..."
	@rm -rf $(OBJDIR) $(BINDIR) $(LIBDIR)

help:
	@printf "Makefile targets:\n"
	@printf "  make (or make debug)    - build debug binary\n"
	@printf "  make release            - build optimized release binary\n"
	@printf "  make SANITIZE=1 debug   - build with ASan/UBSan\n"
	@printf "  make run ARGS=\"...\"    - run binary with ARGS\n"
	@printf "  make perft PERFT_ARGS=\"...\" - run perft (if supported)\n"
	@printf "  make clean              - remove build artifacts\n\n"
