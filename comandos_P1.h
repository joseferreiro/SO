#ifndef UNTITLED_COMANDOS_P1_H
#define UNTITLED_COMANDOS_P1_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <sys/utsname.h>
#include <fcntl.h>
#include "lista_comandos.h"
#include "lista_ficheros.h"
#include "lista_memoria.h"
#include "comandos_P0.h"
#include "comandos_P3.h"

#include <sys/stat.h>
#include <pwd.h>
#include <grp.h>
#include <limits.h>
#include <errno.h>
#include <dirent.h>
#include <ctype.h>
#include <sys/wait.h>

#include <sys/mman.h>
#include <sys/shm.h>
#include <stdint-gcc.h>

#include <sys/resource.h>

void create(char *command, char *name);
void stat_fun(char *tr[]);
void list_fun(char *tr[]);
void delete(char *paths[]);
void deltree(char *paths[]);

#endif //UNTITLED_COMANDOS_P1_H
