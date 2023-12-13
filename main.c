//Autor 1: José Manuel Ferreiro López   login: jose.manuel.ferreiro@udc.es
//Autor 2: Alvaro Mendez Linares    login: alvaro.mendez.linares@udc.es

//AREGLAR LEAK DE MALLOC !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//hacer que el comando se almacene en la lista antes de procesar_entrada

// NombreSenal en lista_procesos.c vs código de ayuda


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include "lista_comandos.h"
#include "lista_ficheros.h"
#include "lista_memoria.h"
#include "comandos_P0.h"
#include "comandos_P1.h"
#include "comandos_P2.h"
#include "comandos_P3.h"

#include <sys/wait.h>

#define TROZOS 15  //numere de trozos que se pueden almacenar en la array trozos

ulong main3arg;      //----------------------
ulong *addrmain3arg; //----------------------

int TrocearCadena(const char *cadena, char *trozos[], int maxTrozos) {
    int i = 0;
    int comilla = 0;
    const char *start = cadena;

    while (*cadena && i < maxTrozos - 1) {
        if (*cadena == '\'') {
            comilla = !comilla;
        }

        if ((*cadena == ' ' && !comilla) || *(cadena + 1) == '\0') {
            int length = cadena - start + 1;  // Include the space or the null terminator
            if (length > 0) {
                // Comprobamos si el trozo de string empieza con ' y la saltamos
                if (*start == '\'') {
                    start++;
                    length--;
                }
                // Comprobamos si termina en ' y la eliminamos
                if (start[length - 2] == '\'') {
                    length--;
                }
                trozos[i] = (char *)malloc(length);
                if (trozos[i] == NULL) {
                    perror("No se pudo procesar el comando");
                    exit(1);
                }
                strncpy(trozos[i], start, length - 1);
                trozos[i][length - 1] = '\0';
                i++;
            }
            start = cadena + 1;
        }
        cadena++;
    }
    //Terminamos la array en NULL para evitar que las funciones intenten acceder a la últina posición
    trozos[i] = NULL;
    return i;
}

int splano(char *args[], int numArgs, BackgroundProcessList *LP) {
    pid_t pid;

    switch (pid = fork()) {
        case -1:
            perror("fork");
            return -1;
        case 0: // Child process
            if (execvp(args[0], args) == -1) {
                perror("execvp");
                exit(EXIT_FAILURE);
            }
            // It's a good practice to add exit() after exec failure to ensure the child process exits
        default: // Parent process
            // Add the child process to the list of background processes
            addBackgroundProcess(LP, pid, args[0]);
            return 0;
    }
}

int cmdexec(char *args[]) {
    pid_t pid;

    switch (pid = fork()) {
        case -1:
            perror("fork");
            return -1;
        case 0: // Child process
            if (execvp(args[0], args) == -1) {
                perror("execvp");
                exit(EXIT_FAILURE);
            }
            // It's a good practice to add exit() after exec failure to ensure the child process exits
        default: // Parent process
            waitpid(pid, NULL, 0); // Wait for the child process to complete
            return 0;
    }
}

int runInBackground(char *args[], int numArgs, BackgroundProcessList *LP) {
    pid_t pid;

    switch (pid = fork()) {
        case -1:
            perror("fork");
            return -1;
        case 0: // Child process
            if (execvp(args[0], args) == -1) {
                perror("execvp");
                exit(EXIT_FAILURE);
            }
            // It's a good practice to add exit() after exec failure to ensure the child process exits
        default: // Parent process
            // Add the child process to the list of background processes
            addBackgroundProcess(LP, pid, args[0]);
            return 0;
    }
}

void executeCommand(char *trozos[], int numt, int runInBackground, tListMem *listamemoria, BackgroundProcessList *LP) {
    // Create an array for the arguments, including the program name
    char *args[numt + 1];
    for (int i = 0; i < numt; i++) {
        args[i] = trozos[i];
    }
    args[numt] = NULL;  // Null-terminate the argument array

    // Execute the command
    int result = runInBackground ? splano(args, numt, LP) : cmdexec(args);

    if (result == -1) {
        perror("Error executing command");
    }
}

void procesar_entrada(char *command, tListCom *listcom, tListFic *listafic, tListMem *listamemoria, BackgroundProcessList *LP, int *exit, int storeCommand) {
    char *trozos[TROZOS];

    if(storeCommand)
        InsertCommand(listcom, command);
    int numt = TrocearCadena(command, trozos, TROZOS);
    if (numt > 0) {
        if (!strcmp(trozos[0], "authors")) {
            authors(trozos[1]);

        } else if (!strcmp(trozos[0], "pid")) {
            pid(trozos[1]);

        } else if (!strcmp(trozos[0], "chdir")) {
            chdir_fun(trozos[1]);

        } else if (!strcmp(trozos[0], "date")) {
            date();

        } else if (!strcmp(trozos[0], "time")) {
            time_fun();

        } else if (!strcmp(trozos[0], "hist")) {
            hist(trozos[1], listcom);

        } else if (!strcmp(trozos[0], "command")) {
            commandN(trozos[1], listcom, listafic, listamemoria, LP, exit);

        } else if (!strcmp(trozos[0], "open")) {
            Cmd_open(trozos, *listafic);

        } else if (!strcmp(trozos[0], "close")) {
            Cmd_close(trozos[1], *listafic);

        } else if (!strcmp(trozos[0], "dup")) {
            Cmd_dup( trozos[1], *listafic);

        } else if (!strcmp(trozos[0], "listopen")) {
            listopen(listafic);

        } else if (!strcmp(trozos[0], "infosys")) {
            infosys();

        } else if (!strcmp(trozos[0], "help")) {
            help(trozos[1]);

        } else if (!strcmp(trozos[0], "quit") || !strcmp(trozos[0], "exit") || !strcmp(trozos[0], "bye")) {
            *exit = 1;

        } else  if (!strcmp(trozos[0], "create")){
            create(trozos[1], trozos[2]);

        } else  if (!strcmp(trozos[0], "stat")){
            stat_fun(trozos + 1);

        }else  if (!strcmp(trozos[0], "list")){
            list_fun(trozos + 1);

        } else  if (!strcmp(trozos[0], "delete")){
            delete(trozos + 1);

        } else  if (!strcmp(trozos[0], "deltree")){
            deltree(trozos + 1);

        } else  if (!strcmp(trozos[0], "malloc")){
            malloc_fun(trozos + 1, listamemoria);

        } else  if (!strcmp(trozos[0], "shared")){
            Shared(trozos + 1, listamemoria);

        } else  if (!strcmp(trozos[0], "mmap")){
            mmap_fun(trozos + 1, listamemoria);

        } else  if (!strcmp(trozos[0], "read")){
            read_fun((trozos + 1));

        } else  if (!strcmp(trozos[0], "write")){
            write_fun(trozos + 1, listamemoria);

        } else  if (!strcmp(trozos[0], "memdump")){
            memdump((trozos + 1));

        } else  if (!strcmp(trozos[0], "memfill")){
            memfill((long *) (trozos + 1));

        } else  if (!strcmp(trozos[0], "mem")){
            mem((trozos + 1), listamemoria);

        } else  if (!strcmp(trozos[0], "recurse")){
            Recurse(trozos + 1);

        } else  if (!strcmp(trozos[0], "uid")){
            CommandUid(trozos + 1);

        }  else  if (!strcmp(trozos[0], "showenv")){
            CommandShowEnvironment(trozos + 1);

        } else  if (!strcmp(trozos[0], "showvar")){
            CommandShowVariable(trozos + 1);

        } else  if (!strcmp(trozos[0], "changevar")){
            CommandChangeVariable(trozos + 1);

        } else  if (!strcmp(trozos[0], "subsvar")){
            CommandSubstituteVariable(trozos + 1);

        } else  if (!strcmp(trozos[0], "fork")){
            ExecuteInBackground(LP);

        } else  if (!strcmp(trozos[0], "exec")){
            ExecuteCommandWrapper((ulong) (trozos + 1));

        } else  if (!strcmp(trozos[0], "jobs")){
            ShowJobs(LP);

        } else  if (!strcmp(trozos[0], "deljobs")){
            DeleteJobs(trozos + 1, LP);

        } else  if (!strcmp(trozos[0], "job")){
            JobCommand(trozos + 1, LP);

        } else {
            int runInBackground = (numt > 1 && strcmp(trozos[numt - 1], "&") == 0);
            executeCommand(trozos, numt - runInBackground, runInBackground, listamemoria, LP);
        }
        for (int i = 0; i < numt; i++) {
            free(trozos[i]);
        }
    }
}


int main(int argc, char const *argv[], char *envp[]) {
    char command[MAXLEN];
    int exit = 0;
    int storeCommand = 1;

    tListCom list;
    tListFic listaficheros;
    tListMem listamemoria;
    BackgroundProcessList LP;
    CreateListCom(&list);
    CreateListFic(&listaficheros);
    CreateListMemBlock(&listamemoria);
    initProcessList(&LP);
    ulong var = (ulong) envp;

    main3arg = var;
    addrmain3arg = &var;

    AddFic(&listaficheros, 0, O_RDWR, "entrada estandar");
    AddFic(&listaficheros, 1, O_RDWR, "salida estandar");
    AddFic(&listaficheros, 2, O_RDWR, "error estandar");

    while (exit == 0){
        imprimirPrompt();
        leerEntrada(command);
        procesar_entrada(command, &list, &listaficheros, &listamemoria, &LP, &exit, storeCommand);
    }
    ClearListFic(&listaficheros);
    ClearListCom(&list);
    ClearListMemBlock(&listamemoria);
    cleanupProcessList(&LP);
    return 0;
}

void print_funcs(void) {
    printf("Funciones de programa: %18p, %18p, %18p\nFunciones de libreria: %18p, %18p, %18p\n",
           main, TrocearCadena, procesar_entrada, printf, fgets, malloc);
}