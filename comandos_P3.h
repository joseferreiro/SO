#ifndef UNTITLED_COMANDOS_P3_H
#define UNTITLED_COMANDOS_P3_H

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

void CommandUid(char **commandArgs);
void CommandShowEnvironment(char **commandArgs);
void CommandShowVariable(char **commandArgs);
void CommandChangeVariable(char **commandParams);
void CommandSubstituteVariable(char **commandParams);
void ExecuteInBackground(BackgroundProcessList *processList);
void ExecuteCommandWrapper(ulong param);
void ShowJobs(BackgroundProcessList* processList);
void DeleteJobs(char** options, BackgroundProcessList* processList);
void JobCommand(char** options, BackgroundProcessList* processList);

#endif //UNTITLED_COMANDOS_P3_H
