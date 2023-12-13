#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "comandos_P3.h"

#include <pwd.h>
#include <errno.h>
#include <sys/wait.h>

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

char *GetUsernameByUid(__uid_t uid)
{
    struct passwd *userEntry = getpwuid(uid);
    if (userEntry == NULL) {
        return " ??????";
    } else {
        return userEntry->pw_name;
    }
}

__uid_t GetUidByUsername(char *username)
{
    struct passwd *userEntry = getpwnam(username);
    if (userEntry == NULL) {
        return (__uid_t)-1;
    } else {
        return userEntry->pw_uid;
    }
}

void CommandGetUid()
{
    __uid_t realUid = getuid();
    __uid_t effectiveUid = geteuid();

    printf("Real credential: %lu, (%s)\n", (ulong)realUid, GetUsernameByUid(realUid));
    printf("Effective credential: %lu, (%s)\n", (ulong)effectiveUid, GetUsernameByUid(effectiveUid));
}

void CommandUid(char **commandArgs)
{
    char **commandParams;
    int comparisonResult;
    int *errorPointer;
    char *errorMessage;
    __uid_t targetUid;

    //(char) 0x0 en lugar de NULL para que no proteste el compilador
    if (*commandArgs == NULL || (comparisonResult = strcmp(*commandArgs, "-get"), comparisonResult == 0)
        || *commandArgs[1] == (char)0x0 || (comparisonResult = strcmp(commandArgs[1], "-l"), comparisonResult == 0 && *commandArgs[2] == (char)0x0))
    {
        CommandGetUid();
    } else {
        comparisonResult = strcmp(*commandArgs, "-get");
        if (comparisonResult == 0) {
            puts("uid [-get|-set] [-l] [id]");
        } else {
            commandParams = commandArgs + 1;
            comparisonResult = strcmp(*commandParams, "-l");
            if (comparisonResult == 0) {
                targetUid = GetUidByUsername(commandArgs[2]);
                if (targetUid == (__uid_t)-1) {
                    printf("Nonexistent user: %s\n", commandArgs[2]);
                    return;
                }
            } else {
                targetUid = atoi(*commandParams);
                if (targetUid < 0) {
                    targetUid = (__uid_t)-1;
                }
                if (targetUid == (__uid_t)-1) {
                    printf("Invalid credential value: %s\n", *commandParams);
                    return;
                }
            }
            comparisonResult = seteuid(targetUid);
            if (comparisonResult == -1) {
                errorPointer = __errno_location();
                errorMessage = strerror(*errorPointer);
                printf("Unable to change credential: %s\n", errorMessage);
            }
        }
    }
}

//-------------------------------------

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

void CommandShowEnvironment(char **commandArgs)
{
    int comparisonResult;

    if (*commandArgs == NULL) {
        ShowEnvironmentVariables((long)main3arg, "main arg3");
    }
    else {
        comparisonResult = strcmp(*commandArgs, "-environ");
        if (comparisonResult == 0) {
            ShowEnvironmentVariables((long)environ, "environ");
        }
        else {
            comparisonResult = strcmp(*commandArgs, "-addr");
            if (comparisonResult == 0) {
                printf("environ:   %p (stored at %p)\nmain arg3: %p (stored at %p)\n",
                       environ, &environ, (char *)main3arg, addrmain3arg);
            }
            else {
                printf("Usage: %s [-environ|-addr]\n", commandArgs[-1]);
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

void CommandShowVariable(char **commandArgs)
{
    int comparisonResult;
    char *environmentVariable;

    if (*commandArgs == NULL) {
        CommandShowEnvironment(commandArgs);
    }
    else {
        comparisonResult = SearchVariable(*commandArgs, (char **) main3arg);
        if (comparisonResult != -1) {
            printf("With arg3 main %s (%p) @%p\n",
                   (char *)*(ulong *)(main3arg + (long)comparisonResult * 8),
                   (char *)*(ulong *)(main3arg + (long)comparisonResult * 8),
                   (char *)main3arg + (long)comparisonResult * 8);
        }
        comparisonResult = SearchVariable(*commandArgs, environ);
        if (comparisonResult != -1) {
            printf("With environ %s (%p) @%p\n",
                   environ[comparisonResult],
                   environ[comparisonResult],
                   &environ[comparisonResult]);
        }
        environmentVariable = getenv(*commandArgs);
        if (environmentVariable != (char *)0x0) {
            printf("With getenv %s (%p)\n", environmentVariable, environmentVariable);
        }
    }
}

//----------------------------------

int FindVariableIndex(const char *targetVariable, long arrayAddress) {
    if (targetVariable == NULL) {
        errno = EINVAL; // Invalid argument
        return -1;
    }

    int index = 0;
    size_t targetLength;
    char buffer[BUFSIZ + 1];  // Increase buffer size by 1 for null terminator

    // Copy the target variable to a local buffer
    targetLength = strlen(targetVariable);
    if (targetLength >= BUFSIZ) {
        // Handle buffer overflow
        errno = EINVAL; // Invalid argument
        return -1;
    }

    strncpy(buffer, targetVariable, BUFSIZ);
    buffer[targetLength] = '\0';  // Ensure null termination

    // Append the terminator character '=' to the buffer
    targetLength = strlen(buffer);
    buffer[targetLength] = '=';

    while (*(char **)(arrayAddress + (long)index * sizeof(char *)) != NULL) {
        // Compare the target variable with the current element in the array
        targetLength = strlen(buffer);
        int comparisonResult = strncmp(*(char **)(arrayAddress + (long)index * sizeof(char *)), buffer, targetLength);

        // If the comparison is successful, break out of the loop
        if (comparisonResult == 0) {
            return index;
        }

        // Move to the next element in the array
        index++;
    }

    // Set errno to indicate that the variable was not found
    errno = ENOENT; // No such file or directory
    return -1;
}


int ChangeVariable(char *variable, char *value, long arrayAddress)
{
    if (variable == NULL || value == NULL) {
        errno = EINVAL; // Invalid argument
        return -1;
    }

    int index = FindVariableIndex(variable, arrayAddress);
    if (index == -1) {
        errno = ENOENT; // No such file or directory
        return -1;
    }

    // Create a new string with the format "variable=value"
    char *newVariable = (char *)malloc(strlen(variable) + strlen(value) + 2);
    if (newVariable == NULL) {
        errno = ENOMEM; // Out of memory
        return -1;
    }

    sprintf(newVariable, "%s=%s", variable, value);

    // Modify the environment directly
    if (setenv(variable, value, 1) != 0) {
        free(newVariable);
        return -1;
    }

    // Free the allocated memory, as it's no longer needed
    free(newVariable);

    return index;
}

int ChangeVariablePutenv(char *variable, char *value) {
    if (variable == NULL || value == NULL) {
        perror("Invalid argument");
        return -1;
    }

    // Create a new string with the format "variable=value"
    char *newVariable = malloc(strlen(variable) + strlen(value) + 2);
    if (newVariable == NULL) {
        perror("Error allocating memory");
        return -1;
    }

    // Set the variable in the environment using putenv
    strcpy(newVariable, variable);
    strcat(newVariable, "=");
    strcat(newVariable, value);

    // Set the variable in the environment using putenv
    if (putenv(newVariable) != 0) {
        perror("Error setting environment variable");
        free(newVariable);
        return -1;
    }
    free(newVariable);
    return 0;
}

void CommandChangeVariable(char **commandParams)
{
    int comparisonResult;
    int *errorPointer;
    char *errorMessage;
    int result;

    if (*commandParams == NULL || commandParams[1] == NULL || commandParams[2] == NULL) {
        printf("Usage: %s [-a|-e|-p] variable value\n", commandParams[-1]);
    } else {
        comparisonResult = strcmp(*commandParams, "-a");
        if (comparisonResult == 0) {
            result = ChangeVariable(commandParams[1], commandParams[2], (long)main3arg);
        } else {
            comparisonResult = strcmp(*commandParams, "-e");
            if (comparisonResult == 0) {
                result = ChangeVariable(commandParams[1], commandParams[2], (long)environ);
            } else {
                comparisonResult = strcmp(*commandParams, "-p");
                if (comparisonResult != 0) {
                    printf("Usage: %s [-a|-e|-p] variable value\n", commandParams[-1]);
                    return;
                }
                result = ChangeVariablePutenv(commandParams[1], commandParams[2]);
            }
        }
        if (result == -1) {
            errorPointer = __errno_location();
            errorMessage = strerror(*errorPointer);
            printf("Unable to change variable: %s\n", errorMessage);
        }
    }
}

//----------------------------------------------

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
            *errorPointer = ENOENT; // No such file or directory
            return -1;
        } else {
            variableLength = strlen(variable);
            newLength = strlen(newValue);
            destination = (char *)malloc(newLength + variableLength + 2);
            if (destination == NULL) {
                errorPointer = __errno_location();
                *errorPointer = ENOMEM; // Out of memory
                return -1;
            } else {
                strcpy(destination, variable);
                variableLength = strlen(destination);
                destination[variableLength] = '='; // '='
                strcpy(destination + variableLength + 1, newValue);
                ((char **)(arrayAddress))[index] = destination;
            }
        }
    } else {
        errorPointer = __errno_location();
        *errorPointer = EEXIST; // File exists
        return -1;
    }
    return index;
}

void CommandSubstituteVariable(char **commandParams) {
    int comparisonResult;
    int *errorPointer;
    char *errorMessage;
    int result = 0;

    if (commandParams == NULL || commandParams[1] == NULL || commandParams[2] == NULL || commandParams[3] == NULL) {
        printf("Usage: %s [-a|-e] variable1 variable2 value\n", (commandParams && commandParams[-1]) ? commandParams[-1] : "unknown_command");
    } else {
        comparisonResult = strcmp(commandParams[0], "-a");
        if (comparisonResult == 0) {
            result = ReplaceVariableValue(commandParams[1], commandParams[3], commandParams[2], (long)main3arg);
        } else {
            comparisonResult = strcmp(commandParams[0], "-e");
            if (comparisonResult != 0) {
                printf("Usage: %s [-a|-e] var1 var2 value\n", (commandParams && commandParams[-1]) ? commandParams[-1] : "unknown_command");
                return;
            }
            result = ReplaceVariableValue(commandParams[1], commandParams[3], commandParams[2], (long)environ);
        }
        if (result == -1) {
            errorPointer = __errno_location();
            errorMessage = strerror(*errorPointer);
            printf("Unable to substitute variable %s with %s: %s\n", commandParams[1], commandParams[2], errorMessage);
        }
    }
}

//----------------------------------------------
//Cmd_fork
void ExecuteInBackground(BackgroundProcessList *processList)
{
    pid_t childProcessID;
    uint processID;

    childProcessID = fork();

    if (childProcessID == 0) {
        cleanupProcessList(processList);
        processID = getpid();
        printf("Executing process %d\n", (unsigned int)processID);
    } else if (childProcessID != -1) {
        waitpid(childProcessID, (int *)0x0, 0);
    }
}

//----------------------------------------------

//ExecuteCommand que no termina la shell (terminar)
int ExecuteCommand(char **params, long credentials, int *priority)
{
    pid_t childPID;
    int status;
    int *errorLocation;

    if (*params == NULL) {
        errorLocation = __errno_location();
        *errorLocation = 0xe;
        return -1;
    }

    childPID = fork();

    if (childPID == -1) {
        return -1;
    }

    if (childPID == 0) {
        // Child process
        execvp(*params, params);

        // If execvp fails, print an error message and exit the child process
        perror("Unable to execute");
        exit(EXIT_FAILURE);
    } else {
        // Parent process
        waitpid(childPID, &status, 0);

        // Optionally, you can check the exit status of the child process
        /*
        if (WIFEXITED(status)) {
            int exitStatus = WEXITSTATUS(status);
        }
         */

        return status;
    }
}

//ExecuteCommand que termina la shell (como la de referencia)
/* int ExecuteCommand(char **params, long credentials, int *priority)
{
    int result;
    id_t processID;
    int *errorLocation;
    char *errorMessage;

    if (*params == NULL) {
        errorLocation = __errno_location();
        *errorLocation = 0xe;
        result = -1;
    } else {
        processID = getpid();

        result = execvp(*params, params);
    }
    return result;
} */

//Cmd_exec
void ExecuteCommandWrapper(ulong param)
{
    int result = ExecuteCommand((char **)param, 0, 0);

    if (result == -1) {
        // Handle the error if needed
        perror("Failed to execute command");
    }
}

//----------------------------------------------

// Utility function to get the name of a signal based on its number
const char* SignalName(int signalNumber) {
    for (int i = 0; sigstrnum[i].nombre != NULL; i++) {
        if (signalNumber == sigstrnum[i].senal) {
            return sigstrnum[i].nombre;
        }
    }
    return "UNKNOWN SIGNAL";
}

// Function to display background jobs
void ShowJobs(BackgroundProcessList* processList) {
    UpdateProcessListStatus(processList);
    listBackgroundProcesses(processList);
}

// Function to delete background jobs based on options
void DeleteJobs(char** options, BackgroundProcessList* processList) {
    for (int i = 0; options[i] != NULL; i++) {
        if (strcmp(options[i], "-term") == 0) {
            deleteFinishedProcesses(processList);
        } else if (strcmp(options[i], "-sig") == 0) {
            deleteSignaledProcesses(processList);
        } else if (strcmp(options[i], "-all") == 0) {
            deleteFinishedProcesses(processList);
            deleteSignaledProcesses(processList);
        } else if (strcmp(options[i], "-clear") == 0) {
            cleanupProcessList(processList);
            initProcessList(processList);
        }
    }

    ShowJobs(processList);
}

// Function to display information about a background job
void JobCommand(char** options, BackgroundProcessList* processList) {
    int foreground = 0;
    char** localOptions = options;

    // Check if the command specifies running in the foreground
    if (*options != NULL && strcmp(*options, "-fg") == 0) {
        foreground = 1;
        localOptions = options + 1;
    }

    // Extract the process ID from options
    pid_t pid = (*localOptions != NULL) ? atoi(*localOptions) : 0;

    // If PID is 0 or not found, display all background jobs
    if (pid == 0 || findProcessIndexByPid(processList, pid) == -1) {
        deleteFinishedProcesses(processList);
        deleteSignaledProcesses(processList);
        listBackgroundProcesses(processList);
        return;
    }

    // If running in the foreground, display detailed information
    if (foreground == 0) {
        showProcessInfoForeground(processList, pid);
        return;
    }

    // Check if the process is already finished
    int exitStatus;
    pid_t waitedPid = waitpid(pid, &exitStatus, WNOHANG);
    if (waitedPid == 0) {
        // Process is still running
        return;
    } else if (waitedPid == -1) { //----------------------------------------------------------
        perror("Error while waiting for the process");
        return;
    }

    // Process has finished, update its status
    if (WIFEXITED(exitStatus)) { //----------------------------------------------------------
        printf("Process %d terminated normally. Returned value %d\n", pid, WEXITSTATUS(exitStatus));
        UpdateProcessStatus(pid, processList);
    } else if (WIFSIGNALED(exitStatus)) { //----------------------------------------------------------
        printf("Process %d terminated by signal %s\n", pid, SignalName(WTERMSIG(exitStatus)));
        UpdateProcessStatus(pid, processList);
    }

    ShowJobs(processList);
}