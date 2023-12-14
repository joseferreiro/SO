#ifndef COMANDOS_P2
#define COMANDOS_P2

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include "lista_comandos.h"
#include "lista_ficheros.h"
#include "lista_memoria.h"
#include "comandos_P0.h"
#include "comandos_P1.h"

#include <sys/stat.h>
#include <errno.h>
#include <sys/wait.h>

#include <sys/mman.h>
#include <sys/shm.h>

#define TAMANO 2048

#define MAP_READ 0x02
#define MAP_WRITE 0x04
#define MAP_EXECUTE 0x08

void read_fun(char *ar[]);
void write_fun(char **args, tListMem *lista);
void malloc_fun(char **args, tListMem *lista);
void Recurse(char **args);
void mmap_fun(char **args, tListMem *list);
void Shared(char **args, tListMem *lista);
void memdump(char **args);
void memfill(char **args);
void mem(char **args, tListMem *lista);

#endif
