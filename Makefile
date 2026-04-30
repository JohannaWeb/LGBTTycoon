CC = gcc
NASM = nasm
CFLAGS = -std=c17 -Wall -Wextra -O2 -g -I. $(shell pkg-config --cflags sdl2)
LDFLAGS = $(shell pkg-config --libs sdl2) -lm
NASMFLAGS = -f elf64

SRC_C = main.c game.c entity.c structure.c render.c
SRC_ASM = tick_decay_simd.asm
OBJ = $(SRC_C:.c=.o) $(SRC_ASM:.asm=.o)
TARGET = lgbtycoon

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(OBJ) -o $(TARGET) $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

%.o: %.asm
	$(NASM) $(NASMFLAGS) $< -o $@

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(OBJ) $(TARGET)

.PHONY: all run clean
