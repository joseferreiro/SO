#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "comandos_P3.h"

#include <pwd.h>
#include <errno.h>
#include <sys/wait.h>

char *GetUsernameByUid(__uid_t uid){
    struct passwd *userEntry = getpwuid(uid);
    if (userEntry == NULL) {
        return "";
    } else {
        return userEntry->pw_name;
    }
}

__uid_t GetUidByUsername(char *username){
    struct passwd *userEntry = getpwnam(username);
    if (userEntry == NULL) {
        return (__uid_t)-1;
    } else {
        return userEntry->pw_uid;
    }
}

void CommandGetUid(){
    __uid_t realUid = getuid();
    __uid_t effectiveUid = geteuid();

    printf("Credencial real: %lu, (%s)\n", (ulong)realUid, GetUsernameByUid(realUid));
    printf("Credencial efectiva: %lu, (%s)\n", (ulong)effectiveUid, GetUsernameByUid(effectiveUid));
}

void cmdUid(char **args){
    char **commandParams;
    int comparisonResult;
    __uid_t targetUid;

    //(char) 0x0 en lugar de NULL para que no proteste el compilador
    if (*args == NULL || (comparisonResult = strcmp(*args, "-get"), comparisonResult == 0)
        || *args[1] == (char)0x0 || (comparisonResult = strcmp(args[1], "-l"), comparisonResult == 0 && *args[2] == (char)0x0))
    {
        CommandGetUid();
    } else {
        comparisonResult = strcmp(*args, "-get");
        if (comparisonResult == 0) {
            puts("uid [-get|-set] [-l] [id]");
        } else {
            commandParams = args + 1;
            comparisonResult = strcmp(*commandParams, "-l");
            if (comparisonResult == 0) {
                targetUid = GetUidByUsername(args[2]);
                if (targetUid == (__uid_t)-1) {
                    printf("El usuario %s no existe\n", args[2]);
                    return;
                }
            } else {
                targetUid = atoi(*commandParams);
                if (targetUid == (__uid_t)-1) {
                    printf("Credenciales inválidas: %s\n", *commandParams);
                    return;
                }
            }
            comparisonResult = seteuid(targetUid);
            if (comparisonResult == -1) {
                perror("No se pudo cambiar la credencial");
            }
        }
    }
}



void ShowEnvironmentVariables(long environmentAddress, char *variableName)
{
    uint index = 0;
    while (*(long *)(environmentAddress + (long)(int)index * 8) != 0) {
        printf("%p -> %s[%u] = (%p) %s\n",
               (char *)((long)(int)index * 8 + environmentAddress), variableName, index,
               (char *)*(ulong *)(environmentAddress + (long)(int)index * 8),
               (char *)*(ulong *)(environmentAddress + (long)(int)index * 8));
        index = index + 1;
    }
}

void cmdShowenv(char **args)
{
    int comparisonResult;

    if (*args == NULL) {
        ShowEnvironmentVariables((long)main3arg, "main arg3");
    }
    else {
        comparisonResult = strcmp(*args, "-environ");
        if (comparisonResult == 0) {
            ShowEnvironmentVariables((long)environ, "environ");
        }
        else {
            comparisonResult = strcmp(*args, "-addr");
            if (comparisonResult == 0) {
                printf("environ:   %p (almacenado en %p)\nmain arg3: %p (almacenado en %p)\n",
                       environ, &environ, (char *)main3arg, addrmain3arg);
            }
            else {
                printf("Uso: showenv [-environ|-addr]\n");
            }
        }
    }
}

int SearchVariable (char * var, char **e)  /*busca una variable en el entorno que se le pasa como parámetro*/
{
    int pos=0;
    char aux[4096];

    strcpy (aux,var);
    strcat (aux,"=");

    while (e[pos]!=NULL)
        if (!strncmp(e[pos],aux,strlen(aux)))
            return (pos);
        else
            pos++;
    errno=ENOENT;   /*no hay tal variable*/
    return(-1);
}

void cmdShowvar(char **args)
{
    int comparisonResult;
    char *environmentVariable;

    if (*args == NULL) {
        cmdShowenv(args);
    }
    else {
        comparisonResult = SearchVariable(*args, (char **) main3arg);
        if (comparisonResult != -1) {
            printf("Con arg3 main %s (%p) @%p\n",
                   (char *)*(ulong *)(main3arg + (long)comparisonResult * 8),
                   (char *)*(ulong *)(main3arg + (long)comparisonResult * 8),
                   (char *)main3arg + (long)comparisonResult * 8);
        }
        comparisonResult = SearchVariable(*args, environ);
        if (comparisonResult != -1) {
            printf("Con environ %s (%p) @%p\n",
                   environ[comparisonResult],
                   environ[comparisonResult],
                   &environ[comparisonResult]);
        }
        environmentVariable = getenv(*args);
        if (environmentVariable != (char *)0x0) {
            printf("Con getenv %s (%p)\n", environmentVariable, environmentVariable);
        }
    }
}



int FindVariableIndex(const char *targetVariable, long arrayAddress) {
    if (targetVariable == NULL) {
        errno = EINVAL;
        return -1;
    }

    int index = 0;
    size_t targetLength;
    char buffer[BUFSIZ + 1];

    targetLength = strlen(targetVariable);
    if (targetLength >= BUFSIZ) {
        errno = EINVAL;
        return -1;
    }

    strncpy(buffer, targetVariable, BUFSIZ);
    buffer[targetLength] = '\0';

    targetLength = strlen(buffer);
    buffer[targetLength] = '=';

    while (*(char **)(arrayAddress + (long)index * sizeof(char *)) != NULL) {
        targetLength = strlen(buffer);
        int comparisonResult = strncmp(*(char **)(arrayAddress + (long)index * sizeof(char *)), buffer, targetLength);

        if (comparisonResult == 0) {
            return index;
        }
        index++;
    }

    errno = ENOENT;
    return -1;
}

int ChangeVariable(char *variable, char *value, long arrayAddress){
    if (variable == NULL || value == NULL) {
        errno = EINVAL;
        return -1;
    }

    int index = FindVariableIndex(variable, arrayAddress);
    if (index == -1) {
        errno = ENOENT;
        return -1;
    }

    char *newVariable = (char *)malloc(strlen(variable) + strlen(value) + 2);
    if (newVariable == NULL) {
        errno = ENOMEM;
        return -1;
    }

    sprintf(newVariable, "%s=%s", variable, value);

    if (setenv(variable, value, 1) != 0) {
        free(newVariable);
        return -1;
    }

    free(newVariable);
    return index;
}

int ChangeVariablePutenv(char *variable, char *value) {
    if (variable == NULL || value == NULL) {
        perror("Argumento no válido");
        return -1;
    }

    char *newVariable = malloc(strlen(variable) + strlen(value) + 2);
    if (newVariable == NULL) {
        perror("Error alocando memoria");
        return -1;
    }

    strcpy(newVariable, variable);
    strcat(newVariable, "=");
    strcat(newVariable, value);

    if (putenv(newVariable) != 0) {
        perror("Error modificando la variable de entorno");
        free(newVariable);
        return -1;
    }
    free(newVariable);
    return 0;
}

void cmdChangevar(char **args){
    int comparisonResult;
    int result;

    if (*args == NULL || args[1] == NULL || args[2] == NULL) {
        printf("Uso: %s [-a|-e|-p] variable valor\n", args[-1]);
    } else {
        comparisonResult = strcmp(*args, "-a");
        if (comparisonResult == 0) {
            result = ChangeVariable(args[1], args[2], (long)main3arg);
        } else {
            comparisonResult = strcmp(*args, "-e");
            if (comparisonResult == 0) {
                result = ChangeVariable(args[1], args[2], (long)environ);
            } else {
                comparisonResult = strcmp(*args, "-p");
                if (comparisonResult != 0) {
                    printf("Uso: %s [-a|-e|-p] variable valor\n", args[-1]);
                    return;
                }
                result = ChangeVariablePutenv(args[1], args[2]);
            }
        }
        if (result == -1) {
            perror("No se pudo cambiar la variable");
        }
    }
}



int ReplaceVariableValue(char *variableIndex, const char *newValue, char *variable, long arrayAddress) {
    int index = FindVariableIndex(variable, arrayAddress);
    int *errorPointer;
    size_t variableLength;
    size_t newLength;
    char *destination;

    if (index == -1) {
        index = FindVariableIndex((char *)variableIndex, arrayAddress);
        if (index == -1) {
            errorPointer = __errno_location();
            *errorPointer = ENOENT;
            return -1;
        } else {
            variableLength = strlen(variable);
            newLength = strlen(newValue);
            destination = (char *)malloc(newLength + variableLength + 2);
            if (destination == NULL) {
                errorPointer = __errno_location();
                *errorPointer = ENOMEM;
                return -1;
            } else {
                strcpy(destination, variable);
                variableLength = strlen(destination);
                destination[variableLength] = '=';
                strcpy(destination + variableLength + 1, newValue);
                ((char **)(arrayAddress))[index] = destination;
            }
        }
    } else {
        errorPointer = __errno_location();
        *errorPointer = EEXIST;
        return -1;
    }
    return index;
}

void cmdSubsvar(char **args) {
    int comparisonResult;
    int *errorPointer;
    char *errorMessage;
    int result;

    if (args == NULL || args[1] == NULL || args[2] == NULL || args[3] == NULL) {
        printf("Uso: subsvar [-a|-e] variable1 variable2 valor\n");
    } else {
        comparisonResult = strcmp(args[0], "-a");
        if (comparisonResult == 0) {
            result = ReplaceVariableValue(args[1], args[3], args[2], (long)main3arg);
        } else {
            comparisonResult = strcmp(args[0], "-e");
            if (comparisonResult != 0) {
                printf("Uso: subsvar [-a|-e] variable1 variable2 valor\n");
                return;
            }
            result = ReplaceVariableValue(args[1], args[3], args[2], (long)environ);
        }
        if (result == -1) {
            errorPointer = __errno_location();
            errorMessage = strerror(*errorPointer);
            printf("No se pudo substituir la variable %s con %s: %s\n", args[1], args[2], errorMessage);
        }
    }
}



void ExecuteInBackground(BackgroundProcessList *processList)
{
    pid_t childProcessID;
    uint processID;

    childProcessID = fork();

    if (childProcessID == 0) {
        clearProcessList(processList);
        processID = getpid();
        printf("Ejecutando proceso %d\n", (unsigned int)processID);
    } else if (childProcessID != -1) {
        waitpid(childProcessID, 0, 0);
    }
}

void cmdExec(char **process) {
    int result;

    if (*process == NULL) {
        errno = EINVAL;
        result = -1;
    } else {
        result = execvp(*process, process);
        if (result == -1) {
            perror("execvp");
        }
    }

    if (result == -1) {
        perror("No se pudo ejecutar el proceso");
    }
}



void cmdJobs(BackgroundProcessList* processList) {
    UpdateProcessListStatus(processList);
    listBackgroundProcesses(processList);
}

void cmdDeljobs(char **args, BackgroundProcessList* processList) {
    for (int i = 0; args[i] != NULL; i++) {
        if (strcmp(args[i], "-term") == 0) {
            deleteFinishedProcesses(processList);
        } else if (strcmp(args[i], "-sig") == 0) {
            deleteSignaledProcesses(processList);
        }
    }
    cmdJobs(processList);
}

void cmdJob(char **args, BackgroundProcessList* processList) {
    int foreground = 0;
    char** localOptions = args;

    if (*args != NULL && strcmp(*args, "-fg") == 0) {
        foreground = 1;
        localOptions = args + 1;
    }

    pid_t pid = (*localOptions != NULL) ? atoi(*localOptions) : 0;

    if (pid == 0 || findProcessIndexByPid(processList, pid) == -1) {
        deleteFinishedProcesses(processList);
        deleteSignaledProcesses(processList);
        listBackgroundProcesses(processList);
        return;
    }

    if (foreground == 0) {
        pid_t childPid = fork();
        if (childPid == 0) {
            execvp(localOptions[0], localOptions);
            perror("execvp");
            exit(EXIT_FAILURE);
        } else if (childPid > 0) {
            addBackgroundProcess(processList, childPid, (const char *) localOptions);
        } else {
            perror("fork");
        }
        return;
    }

    int exitStatus;
    pid_t waitedPid = waitpid(pid, &exitStatus, 0);

    if (pid != waitedPid) {
        perror("Error esperando al proceso");
        return;
    }

    int index = findProcessIndexByPid(processList, pid);
    if (index != -1) {
        if (WIFEXITED(exitStatus)) {
            processList->processes[index].status = PROCESS_FINISHED;
            processList->processes[index].return_value = WEXITSTATUS(exitStatus);
            printf("Proceso %d terminado normalmente. Valor devuelto %d\n", pid, WEXITSTATUS(exitStatus));
        } else if (WIFSIGNALED(exitStatus)) {
            processList->processes[index].status = PROCESS_SIGNALED;
            processList->processes[index].return_value = WTERMSIG(exitStatus);
            printf("Proceso %d terminado por señal %s\n", pid, SignalName(WTERMSIG(exitStatus)));
        } else if (WIFSTOPPED(exitStatus)) {
            processList->processes[index].status = PROCESS_STOPPED;
            printf("Proceso %d parado\n", pid);
        }
    }
}



