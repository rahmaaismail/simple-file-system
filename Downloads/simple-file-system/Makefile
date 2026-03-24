# Makefile for Simple File System

CC = gcc
CFLAGS = -Wall -Wextra -g
TARGET = diskinfo

# Source files
SRCS = diskinfo.c fs_utils.c
OBJS = $(SRCS:.c=.o)

all: $(TARGET)

# Compile target
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS)

# Compile .c to .o
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Clean up object files and executable
clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: all clean