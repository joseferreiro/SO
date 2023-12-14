#include "lista_procesos.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct SEN{
    char *nombre;
    int senal;
};

/*las siguientes funciones nos permiten obtener el nombre de una senal a partir
del número y viceversa */
static struct SEN sigstrnum[]={
        {"HUP", SIGHUP},
        {"INT", SIGINT},
        {"QUIT", SIGQUIT},
        {"ILL", SIGILL},
        {"TRAP", SIGTRAP},
        {"ABRT", SIGABRT},
        {"IOT", SIGIOT},
        {"BUS", SIGBUS},
        {"FPE", SIGFPE},
        {"KILL", SIGKILL},
        {"USR1", SIGUSR1},
        {"SEGV", SIGSEGV},
        {"USR2", SIGUSR2},
        {"PIPE", SIGPIPE},
        {"ALRM", SIGALRM},
        {"TERM", SIGTERM},
        {"CHLD", SIGCHLD},
        {"CONT", SIGCONT},
        {"STOP", SIGSTOP},
        {"TSTP", SIGTSTP},
        {"TTIN", SIGTTIN},
        {"TTOU", SIGTTOU},
        {"URG", SIGURG},
        {"XCPU", SIGXCPU},
        {"XFSZ", SIGXFSZ},
        {"VTALRM", SIGVTALRM},
        {"PROF", SIGPROF},
        {"WINCH", SIGWINCH},
        {"IO", SIGIO},
        {"SYS", SIGSYS},
/*senales que no hay en todas partes*/
#ifdef SIGPOLL
        {"POLL", SIGPOLL},
#endif
#ifdef SIGPWR
        {"PWR", SIGPWR},
#endif
#ifdef SIGEMT
        {"EMT", SIGEMT},
#endif
#ifdef SIGINFO
        {"INFO", SIGINFO},
#endif
#ifdef SIGSTKFLT
        {"STKFLT", SIGSTKFLT},
#endif
#ifdef SIGCLD
        {"CLD", SIGCLD},
#endif
#ifdef SIGLOST
        {"LOST", SIGLOST},
#endif
#ifdef SIGCANCEL
        {"CANCEL", SIGCANCEL},
#endif
#ifdef SIGTHAW
        {"THAW", SIGTHAW},
#endif
#ifdef SIGFREEZE
        {"FREEZE", SIGFREEZE},
#endif
#ifdef SIGLWP
        {"LWP", SIGLWP},
#endif
#ifdef SIGWAITING
        {"WAITING", SIGWAITING},
#endif
        {NULL,-1},
};    /*fin array sigstrnum */

const char* SignalName(int signalNumber) {
    for (int i = 0; sigstrnum[i].nombre != NULL; i++) {
        if (signalNumber == sigstrnum[i].senal) {
            return sigstrnum[i].nombre;
        }
    }
    return "SIGUNKNOWN";
}

void initProcessList(BackgroundProcessList *processList) {
    processList->processes = NULL;
    processList->size = 0;
    processList->capacity = 0;
}

void cleanupProcessList(BackgroundProcessList *processList) {
    if (processList->processes != NULL) {
        free(processList->processes);
    }
}

void addBackgroundProcess(BackgroundProcessList *processList, pid_t pid, const char *commandLine) {
    if (processList->size >= processList->capacity) {
        processList->capacity = (processList->capacity == 0) ? 1 : processList->capacity * 2;
        BackgroundProcess *newProcesses = realloc(processList->processes, processList->capacity * sizeof(BackgroundProcess));

        if (newProcesses == NULL) {
            perror("Error reallocating memory");
            exit(EXIT_FAILURE);
        }

        // Update the processes pointer only if realloc is successful
        processList->processes = newProcesses;
    }

    int index = processList->size;
    processList->processes[index].pid = pid;
    processList->processes[index].parent_pid = getpid(); // Set parent_pid to the current process ID
    processList->processes[index].status = PROCESS_ACTIVE;
    processList->processes[index].return_value = 0;
    processList->processes[index].launch_time = time(NULL);  // Set launch_time to current time

    // Use strncpy to copy the commandLine, but ensure null-termination
    strncpy(processList->processes[index].command_line, commandLine, sizeof(processList->processes[index].command_line) - 1);
    processList->processes[index].command_line[sizeof(processList->processes[index].command_line) - 1] = '\0';

    processList->size++;
}

int findProcessIndexByPid(BackgroundProcessList *processList, pid_t pid) {
    for (int i = 0; i < processList->size; i++) {
        if (processList->processes[i].pid == pid) {
            return i;
        }
    }
    return -1;
}

void removeProcessByIndex(BackgroundProcessList *processList, int index) {
    if (index < 0 || index >= processList->size) {
        return;  // Invalid index
    }

    // Shift elements to fill the gap
    for (int i = index; i < processList->size - 1; i++) {
        processList->processes[i] = processList->processes[i + 1];
    }

    processList->size--;

    // After shifting, ensure that the command_line of the last element is set to NULL
    processList->processes[processList->size].command_line[0] = '\0';
}

void UpdateProcessStatus(pid_t pid, BackgroundProcessList *processList) {
    int status;
    pid_t waitedPid = waitpid(pid, &status, WNOHANG | WUNTRACED);

    int index = findProcessIndexByPid(processList, pid);
    if (index != -1) {
        if (waitedPid == 0) {
            processList->processes[index].status = PROCESS_ACTIVE;
        } else if (waitedPid == -1) {
            perror("Error esperando el proceso");
        } else {
            // Process has finished
            if (WIFEXITED(status)) {
                processList->processes[index].status = PROCESS_FINISHED;
                processList->processes[index].return_value = WEXITSTATUS(status);
                printf("Proceso %d terminado normalmente. Valor devuelto %d\n", pid, WEXITSTATUS(status));
            } else if (WIFSIGNALED(status)) {
                processList->processes[index].status = PROCESS_SIGNALED;
                processList->processes[index].return_value = WTERMSIG(status);
                printf("Proceso %d terminado por señal %s\n", pid, SignalName(WTERMSIG(status)));
            } else if (WIFSTOPPED(status)) {
                processList->processes[index].status = PROCESS_STOPPED;
                printf("Proceso %d parado\n", pid);
            }
        }
    }
}

void UpdateProcessListStatus(BackgroundProcessList *processList) {
    for (int i = 0; i < processList->size; i++) {
        UpdateProcessStatus(processList->processes[i].pid, processList);
    }
}

void printProcessInfo(BackgroundProcess *process) {
    // Check if process is NULL (added for safety)
    if (process == NULL) {
        fprintf(stderr, "Error: NULL process passed to printProcessInfo\n");
        return;
    }

    printf("%5d %-10s p=%-5d %s", process->pid, getpwuid(getuid())->pw_name, process->return_value,
           process->status == PROCESS_ACTIVE ? "ACTIVO" :
           (process->status == PROCESS_FINISHED ? "TERMINADO" :
            (process->status == PROCESS_SIGNALED ? "SIGNALED" : "STOPPED")));

    // Check if launch_time is a valid time
    if (process->launch_time > 0) {
        struct tm *tm_info;
        char buffer[26];

        tm_info = localtime(&process->launch_time);

        // Check if tm_info is not NULL (added for safety)
        if (tm_info != NULL) {
            strftime(buffer, 26, " %Y/%m/%d %H:%M:%S", tm_info);
            printf("%s", buffer);
        } else {
            fprintf(stderr, "Error: Unable to get local time information\n");
        }
    } else {
        fprintf(stderr, "Error: Invalid launch_time in process\n");
    }

    printf(" (%03d) %s\n", process->return_value, process->command_line);
}

void listBackgroundProcesses(BackgroundProcessList *processList) {
    for (int i = 0; i < processList->size; i++) {
        printProcessInfo(&(processList->processes[i]));
    }
}

void deleteFinishedProcesses(BackgroundProcessList *processList) {
    for (int i = processList->size - 1; i >= 0; i--) {
        if (processList->processes[i].status == PROCESS_FINISHED) {
            removeProcessByIndex(processList, i);
        }
    }
}

void deleteSignaledProcesses(BackgroundProcessList *processList) {
    for (int i = processList->size - 1; i >= 0; i--) {
        if (processList->processes[i].status == PROCESS_SIGNALED) {
            removeProcessByIndex(processList, i);
        }
    }
}