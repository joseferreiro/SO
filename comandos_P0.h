#ifndef COMANDOS_P0
#define COMANDOS_P0

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
#include "comandos_P3.h"
#include <ctype.h>

#define MAXLEN 200

void authors(char *command);
void pid(char *commad);
void chdir_fun(char *command);
void date();
void time_fun();
void hist(char *command, tListCom *list);
void commandN(char *command, tListCom *list, tListFic *listfic, tListMem *listamemoria, BackgroundProcessList *LP, int *exit);
void Cmd_open(char *tr[], tListFic listaficheros);
void Cmd_close(char *command, tListFic list);
void Cmd_dup(char *command, tListFic list);
void listopen(tListFic *list);
void infosys();
void help(char *command);
void imprimirPrompt();
void leerEntrada(char *command);

#endif
