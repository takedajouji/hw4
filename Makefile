# Compiler
CC = gcc

# Compiler flags
CFLAGS = -pthread

# Source files
SRCS = hw4.c

# Output executable
TARGET = hw4

# Default target
all: $(TARGET)

# Build the target
$(TARGET): $(SRCS)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRCS)

# Clean up generated files
clean:
	rm -f $(TARGET) average.txt