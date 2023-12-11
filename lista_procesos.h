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


// Structure to represent a background process
typedef struct {
    pid_t pid;              // Process ID
    time_t launch_time;     // Date and time of launching
    ProcessStatus status;             // Process status (FINISHED, STOPPED, SIGNALED, ACTIVE)
    int return_value;       // Return value or signal when relevant
    char command_line[256]; // Command line used to launch the process
    int priority;           // Priority of the process
} BackgroundProcess;

// Structure to represent the list of background processes
typedef struct {
    BackgroundProcess* processes; // Array of background processes
    int size;                     // Current number of processes in the list
    int capacity;                 // Capacity of the array (allocated size)
} BackgroundProcessList;


void CreateProcList(BackgroundProcessList *list);
// Function to clear the list of processes
void VaciarListaProcesos(BackgroundProcessList* processList);

// Function to update the status of all processes in the list
void ActualizarListaProcesos(BackgroundProcessList* processList);

// Function to print information about processes based on their state
void ImprimirListaProcesos(BackgroundProcessList* processList, int state);

// Function to delete processes based on their state
void BorrarProcesosEstado(BackgroundProcessList* processList, int state);

// Function to print information about a process based on its PID
void MostrarProcesoPid(BackgroundProcessList* processList, pid_t pid);

// Function to delete a process based on its PID
unsigned int BorrarProcesoPid(BackgroundProcessList* processList, pid_t pid);

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

