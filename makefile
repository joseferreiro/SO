# Compiler and flags
CC = gcc
CFLAGS = -Wall

# Source files
SRCS = lista_comandos.c lista_ficheros.c lista_memoria.c lista_procesos.c comandos_P0.c comandos_P3.c main.c

# Object files
OBJS = $(SRCS:.c=.o)

# Header files
DEPS = lista_comandos.h lista_ficheros.h lista_memoria.h lista_procesos.h comandos_P0.h comandos_P3.h

# Output executable
OUTPUT = shell

# Default target
all: $(OUTPUT)

$(OUTPUT): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS)

%.o: %.c $(DEPS)
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(OBJS) $(OUTPUT)

