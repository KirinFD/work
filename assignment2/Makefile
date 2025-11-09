# Cross-compile settings
CC = aarch64-linux-gnu-gcc
CFLAGS = -Wall -O2 -pthread -I./include

# Source files
SRC = src/main.c \
      src/periodTimer.c \
      src/pwm_hal.c \
      src/adc_hal.c \
      src/sampler.c \
      src/reporter.c \
      src/SPI.c \
      src/udp_listener.c

# Object files
OBJ = $(SRC:src/%.c=obj/%.o)

# Output binary
BIN = bin/as2

# Default target
all: $(BIN)

# Linking
$(BIN): $(OBJ)
	$(CC) -pthread -o $@ $^

# Compile source files to object files
obj/%.o: src/%.c | obj
	$(CC) $(CFLAGS) -c $< -o $@

# Ensure obj and bin directories exist
obj:
	mkdir -p obj

bin:
	mkdir -p bin

# Clean build
clean:
	rm -rf obj/* bin/*

.PHONY: all clean obj bin

