# Compiler and Flags
CC = gcc
CFLAGS = -Iinclude -Wall -Wextra -std=c11

# Directories
SRC_DIR = src
INCLUDE_DIR = include
LIB_DIR = lib
BIN_DIR = bin

# Targets
LIB_NAME = libmylib.a
EXEC_NAME = myprogram

SRC_FILES = \$(wildcard \$(SRC_DIR)/*.c)
OBJ_FILES = \$(SRC_FILES:.c=.o)
LIB_FILE = \$(LIB_DIR)/\$(LIB_NAME)

all: \$(BIN_DIR)/\$(EXEC_NAME)

\$(LIB_FILE): \$(SRC_DIR)/mylib.o
	mkdir -p \$(LIB_DIR)
	ar rcs \$@ \$^

\$(BIN_DIR)/\$(EXEC_NAME): \$(LIB_FILE) \$(SRC_DIR)/main.o
	mkdir -p \$(BIN_DIR)
	\$(CC) \$(CFLAGS) -o \$@ \$(SRC_DIR)/main.o -L\$(LIB_DIR) -lmylib

\$(SRC_DIR)/%.o: \$(SRC_DIR)/%.c
	\$(CC) \$(CFLAGS) -c \$< -o \$@

clean:
	rm -f \$(SRC_DIR)/*.o \$(LIB_DIR)/*.a \$(BIN_DIR)/*

.PHONY: all clean
