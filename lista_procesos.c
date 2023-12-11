#include "lista_procesos.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>




int ValorSenal(char * sen)  /*devuelve el numero de senial a partir del nombre*/
{
    int i;
    for (i=0; sigstrnum[i].nombre!=NULL; i++)
        if (!strcmp(sen, sigstrnum[i].nombre))
            return sigstrnum[i].senal;
    return -1;
}

void CreateProcList(BackgroundProcessList *list) {
    list->processes = NULL;
    list->size = 0;
    list->capacity = 0;
}

ulong borrarProceso(long param_1, int param_2) {
    ulong *puVar1;
    ulong *puVar2;
    int *piVar3;
    ulong uVar4;
    int local_c;

    if (param_2 < *(int *)(param_1 + 0x8000)) {
        free(*(void **)(param_1 + (long)param_2 * 0x20 + 0x18));
        for (local_c = param_2; local_c < *(int *)(param_1 + 0x8000); local_c = local_c + 1) {
            puVar1 = (ulong *)(param_1 + (long)local_c * 0x20);
            puVar2 = (ulong *)(param_1 + (long)(local_c + 1) * 0x20);
            uVar4 = puVar2[1];
            *puVar1 = *puVar2;
            puVar1[1] = uVar4;
            uVar4 = puVar2[3];
            puVar1[2] = puVar2[2];
            puVar1[3] = uVar4;
        }
        *(int *)(param_1 + 0x8000) = *(int *)(param_1 + 0x8000) + -1;
        uVar4 = 0;
    } else {
        piVar3 = __errno_location();
        *piVar3 = 0x16;
        uVar4 = 0xffffffff;
    }
    return uVar4;
}

// Function to delete processes based on their state
void BorrarProcesosEstado(BackgroundProcessList* processList, int state) {
    uint uVar1;
    uint local_10;
    int local_c;

    local_10 = 0;
    local_c = 0;
    while (local_c < processList->size) {
        if ((processList->processes[local_c].status & state) == 0) {
            local_c = local_c + 1;
        } else {
            uVar1 = BorrarProcesoPid(processList, processList->processes[local_c].pid);
            local_10 = local_10 | uVar1;
            local_c = 0;
        }
    }
}

// Function to clear the list of processes
void VaciarListaProcesos(BackgroundProcessList* processList) {
    BorrarProcesosEstado(processList, PROCESS_ACTIVE);
}

void actualizarProceso(__pid_t *param_1) {
    __pid_t Var1;
    uint local_c;

    Var1 = waitpid(*param_1, (int *)&local_c, 0xb);
    if (Var1 == *param_1) {
        if ((local_c & 0x7f) == 0) {
            param_1[2] = 8;
            param_1[3] = (int)local_c >> 8 & 0xff;
        } else if ((char)(((u_char)local_c & 0x7f) + 1) >> 1 < '\x01') {
            if ((local_c & 0xff) == 0x7f) {
                param_1[2] = 2;
                param_1[3] = (int)local_c >> 8 & 0xff;
            } else if (local_c == 0xffff) {
                param_1[2] = 1;
                param_1[3] = 0;
            }
        } else {
            param_1[2] = 4;
            param_1[3] = local_c & 0x7f;
        }
    }
}

// Function to update the status of all processes in the list
void ActualizarListaProcesos(BackgroundProcessList* processList) {
    BorrarProcesosEstado(processList, PROCESS_FINISHED | PROCESS_STOPPED | PROCESS_SIGNALED);

    for (int local_c = 0; local_c < processList->size; local_c = local_c + 1) {
        actualizarProceso((__pid_t *)(processList->processes + local_c));
    }
}

char* valorEstado(uint param_1) {
    static char result[4];
    sprintf(result, "%03d", (int)param_1);
    return result;
}

char* NombreSenal(int param_1) {
    int local_c = 0;
    while (1) {
        if (*(long *)(sigstrnum + (long)local_c * 0x10) == 0) {
            return "SIGUNKNOWN";
        }
        if (param_1 == *(int *)(sigstrnum + (long)local_c * 0x10 + 8)) break;
        local_c = local_c + 1;
    }
    return *(char **)(sigstrnum + (long)local_c * 0x10);
}

char* nombreEstado(int param_1) {
    if (param_1 == 8) {
        return "TERMINADO";
    }
    if (param_1 < 9) {
        if (param_1 == 4) {
            return "SENALADO";
        }
        if (param_1 < 5) {
            if (param_1 == 1) {
                return "ACTIVO";
            }
            if (param_1 == 2) {
                return "PARADO";
            }
        }
    }
    return "???????";
}

char* fecha(time_t param_1) {
    static char result[20];
    time_t local_20[2];
    struct tm *local_10;
    local_20[0] = param_1;
    local_10 = localtime(local_20);
    sprintf(result, "%4d/%02d/%02d %02d:%02d:%d", (int)(local_10->tm_year + 0x76c),
            (int)(uint)local_10->tm_mon, (int)(uint)local_10->tm_mday,
            (int)(uint)local_10->tm_hour, (int)(uint)local_10->tm_min, (int)(uint)local_10->tm_sec
    );
    return result;
}

char* nombreUsuario(__uid_t param_1) {
    static char result[15];
    struct passwd *ppVar1;
    ppVar1 = getpwuid(param_1);
    if (ppVar1 == (struct passwd *)0x0) {
        sprintf(result, "uid %07d", (int)param_1);
    } else {
        sprintf(result, "%11s", ppVar1->pw_name);
    }
    return result;
}

void imprimirProceso(id_t *param_1) {
    uint priority;
    ulong statusValue;
    char *signalOrState;
    char *processName;

    if (*(long *)(param_1 + 6) == 0) {
        processName = "Unknown Process";  // Replace with an appropriate default name
    } else {
        processName = *(char **)(param_1 + 6);
    }

    if ((param_1[2] & 4) == 0) {
        statusValue = (ulong)valorEstado(param_1[3]);
    } else {
        statusValue = (ulong)NombreSenal(param_1[3]);
    }

    priority = getpriority(PRIO_PROCESS, *param_1);
    signalOrState = (param_1[2] & 4) == 0 ? nombreEstado(param_1[2]) : NombreSenal(param_1[3]);

    printf("%6d %s Priority=%d Launch Time=%s Status=%s (%s) Process Name=%s\n",
           (int)*param_1, nombreUsuario(param_1[1]), (int)priority,
           fecha(*(ulong *)(param_1 + 4)), signalOrState, (char *)statusValue, processName);
}

// Function to print information about processes based on their state
void ImprimirListaProcesos(BackgroundProcessList* processList, int state) {
    for (int local_c = 0; local_c < processList->size; local_c = local_c + 1) {
        if ((processList->processes[local_c].status & state) != 0) {
            imprimirProceso((id_t *)&(processList->processes[local_c]));
        }
    }
}

int BuscarProcesoPid(long param_1, int param_2) {
    int *piVar1;
    int local_c;
    local_c = 0;
    while( 1 ) {
        if (*(int *)(param_1 + 0x8000) <= local_c) {
            piVar1 = __errno_location();
            *piVar1 = 3;
            return -1;
        }
        if (param_2 == *(int *)(param_1 + (long)local_c * 0x20)) break;
        local_c = local_c + 1;
    }
    return local_c;
}

// Function to print information about a process based on its PID
void MostrarProcesoPid(BackgroundProcessList* processList, pid_t pid) {
    int iVar1;
    ulong uVar2;
    iVar1 = BuscarProcesoPid((long)processList, (int)pid);
    if (iVar1 == -1) {
        uVar2 = 0xffffffff;
    } else {
        actualizarProceso((__pid_t *)(processList->processes + (long)iVar1));
        imprimirProceso((id_t *)(processList->processes + (long)iVar1));
        uVar2 = 0;
    }
}

// Function to delete a process based on its PID
unsigned int BorrarProcesoPid(BackgroundProcessList* processList, pid_t pid) {
    int iVar1;
    ulong uVar2;
    iVar1 = BuscarProcesoPid((long)processList, (int)pid);
    if (iVar1 == -1) {
        uVar2 = 0xffffffff;
    } else {
        uVar2 = borrarProceso((long)processList, iVar1);
    }
    return 0;
}
