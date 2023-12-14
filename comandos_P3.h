#ifndef COMANDOS_P3
#define COMANDOS_P3

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lista_procesos.h"

#include <pwd.h>
#include <errno.h>
#include <sys/wait.h>

extern ulong main3arg;
extern ulong *addrmain3arg;
extern char **environ;

void cmdUid(char **args);
void cmdShowenv(char **args);
void cmdShowvar(char **args);
void cmdChangevar(char **args);
void cmdSubsvar(char **args);
void ExecuteInBackground(BackgroundProcessList *processList);
void cmdExec(char **process);
void cmdJobs(BackgroundProcessList *processList);
void cmdDeljobs(char **args, BackgroundProcessList *processList);
void cmdJob(char **args, BackgroundProcessList *processList);

#endif //COMANDOS_P3
