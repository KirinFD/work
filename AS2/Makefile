# Makefile for AS2 project (aarch64 cross-compile)

# Compiler and flags
CC = aarch64-linux-gnu-gcc
CFLAGS = -Wall -O2 -pthread -I./include

# Source files
SRC = src/main.c \
      src/adc_hal.c \
      src/pwm_hal.c \
      src/SPI.c \
      src/sampler.c \
      src/reporter.c \
      src/encoder.c

# Object files
OBJ = $(SRC:src/%.c=obj/%.o)

# Output binary
TARGET = bin/as2_app

# Libraries
LIBS = -lgpiod

# Default target
all: $(TARGET)

# Link
$(TARGET): $(OBJ)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

# Compile source files
obj/%.o: src/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

# Clean
clean:
	rm -rf obj/* $(TARGET)

.PHONY: all clean

