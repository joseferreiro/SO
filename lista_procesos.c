#include "lista_procesos.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void initProcessList(BackgroundProcessList *list) {
    list->processes = NULL;
    list->size = 0;
    list->capacity = 0;
}

void cleanupProcessList(BackgroundProcessList *list) {
    if (list->processes != NULL) {
        free(list->processes);
    }
}

void addBackgroundProcess(BackgroundProcessList *list, pid_t pid, const char *commandLine) {
    if (list->size >= list->capacity) {
        list->capacity = (list->capacity == 0) ? 1 : list->capacity * 2;
        BackgroundProcess *newProcesses = realloc(list->processes, list->capacity * sizeof(BackgroundProcess));

        if (newProcesses == NULL) {
            perror("Error reallocating memory");
            exit(EXIT_FAILURE);
        }

        // Update the processes pointer only if realloc is successful
        list->processes = newProcesses;
    }

    int index = list->size;
    list->processes[index].pid = pid;
    list->processes[index].parent_pid = getpid(); // Set parent_pid to the current process ID
    list->processes[index].status = PROCESS_ACTIVE;
    list->processes[index].return_value = 0;
    list->processes[index].launch_time = time(NULL);  // Set launch_time to current time

    // Use strncpy to copy the commandLine, but ensure null-termination
    strncpy(list->processes[index].command_line, commandLine, sizeof(list->processes[index].command_line) - 1);
    list->processes[index].command_line[sizeof(list->processes[index].command_line) - 1] = '\0';

    list->size++;
}

int findProcessIndexByPid(BackgroundProcessList *list, pid_t pid) {
    for (int i = 0; i < list->size; i++) {
        if (list->processes[i].pid == pid) {
            return i;
        }
    }
    return -1;
}

void removeProcessByIndex(BackgroundProcessList *list, int index) {
    if (index < 0 || index >= list->size) {
        return;  // Invalid index
    }

    // Shift elements to fill the gap
    for (int i = index; i < list->size - 1; i++) {
        list->processes[i] = list->processes[i + 1];
    }

    list->size--;

    // After shifting, ensure that the command_line of the last element is set to NULL
    list->processes[list->size].command_line[0] = '\0';
}

// Function to update the status of a single process
void UpdateProcessStatus(pid_t pid, BackgroundProcessList* processList) {
    int status;
    pid_t waitedPid = waitpid(pid, &status, WNOHANG);

    if (waitedPid == -1) {
        perror("Error while waiting for the process");
        return;
    }

    int index = findProcessIndexByPid(processList, pid);
    if (index != -1) {
        if (waitedPid == 0) {
            // Process is still running
            processList->processes[index].status = PROCESS_ACTIVE;
        } else {
            // Process has finished
            if (WIFEXITED(status)) {
                processList->processes[index].status = PROCESS_FINISHED;
                processList->processes[index].return_value = WEXITSTATUS(status);
            } else if (WIFSIGNALED(status)) {
                processList->processes[index].status = PROCESS_SIGNALED;
                processList->processes[index].return_value = WTERMSIG(status);
            }
        }
    }
}

// Function to update status of all processes in the list
void UpdateProcessListStatus(BackgroundProcessList* processList) {
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


void listBackgroundProcesses(BackgroundProcessList *list) {;
    for (int i = 0; i < list->size; i++) {
        printProcessInfo(&(list->processes[i]));
    }
}

void deleteFinishedProcesses(BackgroundProcessList* list) {
    for (int i = list->size - 1; i >= 0; i--) {
        if (list->processes[i].status == PROCESS_FINISHED) {
            removeProcessByIndex(list, i);
        }
    }
}

void deleteSignaledProcesses(BackgroundProcessList* list) {
    for (int i = list->size - 1; i >= 0; i--) {
        if (list->processes[i].status == PROCESS_SIGNALED) {
            removeProcessByIndex(list, i);
        }
    }
}

void showProcessInfoForeground(BackgroundProcessList *list, pid_t pid) {
    int index = findProcessIndexByPid(list, pid);
    if (index != -1) {
        printProcessInfo(&(list->processes[index]));
        // Move the process to the foreground (simulate waiting for the process to finish)
        UpdateProcessStatus(pid,list);
    } else {
        printf("Process with PID %d not found.\n", pid);
    }
}