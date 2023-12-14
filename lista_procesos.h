#ifndef PROCESS_LIST_H
#define PROCESS_LIST_H

#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <sys/time.h>
#include <sys/resource.h>

typedef enum {
    PROCESS_FINISHED,
    PROCESS_STOPPED,
    PROCESS_SIGNALED,
    PROCESS_ACTIVE
} ProcessStatus;

typedef struct {
    pid_t pid;
    pid_t parent_pid;
    time_t launch_time;
    ProcessStatus status;
    int return_value;
    char command_line[256];
} BackgroundProcess;

typedef struct {
    BackgroundProcess *processes;
    int size;
    int capacity;
} BackgroundProcessList;

void initProcessList(BackgroundProcessList *processList);
void cleanupProcessList(BackgroundProcessList *processList);
void addBackgroundProcess(BackgroundProcessList *processList, pid_t pid, const char *commandLine);
int findProcessIndexByPid(BackgroundProcessList *processList, pid_t pid);
void removeProcessByIndex(BackgroundProcessList *processList, int index);
void UpdateProcessStatus(pid_t pid, BackgroundProcessList *processList);
void printProcessInfo(BackgroundProcess *process);
void listBackgroundProcesses(BackgroundProcessList *processList);
void deleteFinishedProcesses(BackgroundProcessList *processList);
void deleteSignaledProcesses(BackgroundProcessList *processList);
void UpdateProcessListStatus(BackgroundProcessList *processList);
const char* SignalName(int signalNumber);

#endif // PROCESS_LIST_H