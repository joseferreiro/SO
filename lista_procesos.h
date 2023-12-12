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
    pid_t parent_pid;  // Added parent process ID
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

void initProcessList(BackgroundProcessList *list);
void cleanupProcessList(BackgroundProcessList *list);
void addBackgroundProcess(BackgroundProcessList *list, pid_t pid, const char *commandLine);
int findProcessIndexByPid(BackgroundProcessList *list, pid_t pid);
void removeProcessByIndex(BackgroundProcessList *list, int index);
void UpdateProcessStatus(pid_t pid, BackgroundProcessList* processList);
void printProcessInfo(BackgroundProcess *process);
void listBackgroundProcesses(BackgroundProcessList *list);
void deleteFinishedProcesses(BackgroundProcessList *list);
void deleteSignaledProcesses(BackgroundProcessList *list);
void showProcessInfoForeground(BackgroundProcessList *list, pid_t pid);
void UpdateProcessListStatus(BackgroundProcessList* processList);

#endif // PROCESS_LIST_H

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

