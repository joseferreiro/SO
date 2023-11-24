# Compiler and flags
CC = gcc
CFLAGS = -Wall

# Source files
SRCS = lista_comandos.c lista_ficheros.c lista_memoria.c main.c

# Object files
OBJS = $(SRCS:.c=.o)

# Header files
DEPS = lista_comandos.h lista_ficheros.h lista_memoria.h

# Output executable
OUTPUT = p2

# Default target
all: $(OUTPUT)

$(OUTPUT): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS)

%.o: %.c $(DEPS)
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(OBJS) $(OUTPUT)

