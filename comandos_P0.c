#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <sys/utsname.h>
#include <fcntl.h>
#include "lista_comandos.h"
#include "lista_ficheros.h"
#include "lista_memoria.h"
#include "comandos_P0.h"
#include <ctype.h>

int es_numero(const char *str) { //Comprueba si un string es un número
    for (int i = 0; str[i] != '\0'; i++) {
        if (!isdigit(str[i])) {
            return 0;
        }
    }
    return 1;
}

int total_comandos(tListCom list) { //Devuelve el total de comandos de la lista de comandos
    int total = 0;
    tPosCom actual = list;
    while (actual->next != NULL) {
        total++;
        actual = actual->next;
    }
    return total;
}

char *nombre_fichero_descriptor(tListFic list, int df) {
    tPosFic current = list;

    //Mientras que no encuentra y guarda la copia de current->nombre (coincidente con el descriptor de fichero) sigue buscandolo en la lista
    while (current != NULL) {
        if (current->descriptor == df) {
            return strdup(current->nombre);
        }
        current = current->next;
    }

    char *error_message = strdup("Descriptor no encontrado");
    return error_message;
}

void listar_ficheros_abiertos(tListFic *list) {
    tPosFic current = *list;

    while (current != NULL) {
        char *mode_str = "";

        if (current->mode & O_RDONLY) mode_str = "O_RDONLY";
        else if (current->mode & O_EXCL) mode_str = "O_EXCL";
        else if (current->mode & O_WRONLY) mode_str = "O_WRONLY";
        else if (current->mode & O_RDWR) mode_str = "O_RDWR";
        else if (current->mode & O_APPEND) mode_str = "O_APPEND";
        else if (current->mode & O_TRUNC) mode_str = "O_TRUNC";
        else if (current->mode & O_CREAT) mode_str = "O_CREAT";
        else mode_str = "Desconocido";
        printf("descriptor: %d -> %s %s\n", current->descriptor, current->nombre, mode_str);
        current = current->next;
    }
}

void authors(char *command) {
    if (command == NULL) {
        printf("José Manuel Ferreiro López: jose.manuel.ferreiro@udc.es\n");
        printf("Alvaro Mendez Linares : alvaro.mendez.linares@udc.es\n");
    } else {
        if (strcmp(command, "-n") == 0) {
            printf("José Manuel Ferreiro López\n");
            printf("Alvaro Mendez Linares\n");
        } else if (strcmp(command, "-l") == 0) {
            printf("jose.manuel.ferreiro@udc.es\n");
            printf("alvaro.mendez.linares\n");
        } else {
            printf("Error: argumento no reconacido\n");
        }
    }
}

void pid(char *commad) {
    if (commad == NULL) {
        printf("PID del Shell: %i\n", getpid());
    } else {
        if (strcmp(commad, "-p") == 0) {
            pid_t parent_pid = getppid();
            if (parent_pid == -1) {
                perror("Error getting parent process ID");
            } else {
                printf("PID del proceso padre del Shell: %i\n", parent_pid);
            }
        } else {
            printf("Error: argumento no reconocido\n");
        }
    }
}

void chdir_fun(char *command) {
    char temp[MAXLEN];
    if (command == NULL) {
        printf("El directorio actual es: %s\n", getcwd(temp, MAXLEN));
    } else {
        if (chdir(command) == -1) {
            perror("Error cambiando directorio");
        } else {
            printf("Cambiado al directorio: %s\n", getcwd(temp, MAXLEN));
        }
    }
}

void date(){
    time_t t = time(NULL);
    struct tm tm = *localtime(&t);
    printf("%d/%02d/%02d\n", tm.tm_mday, tm.tm_mon + 1,  tm.tm_year + 1900);
}

void time_fun() {
    time_t t = time(NULL);
    struct tm tm = *localtime(&t);
    printf("%02d:%02d:%02d\n", tm.tm_hour, tm.tm_min, tm.tm_sec);
}

void hist(char *command, tListCom *list) {
    if (command == NULL){
        int p = 0;
        for(tPosCom i = *list; i != NULL; i = i->next){
            printf("%d  %s", p, i->command);
            p++;
        }
    }
    else if(!strcmp(command, "-c")){
        ClearListCom(list);
    }
    else{
        int p = 0;
        for(tPosCom i = *list; i != NULL && p <= atoi(command); i = i->next){
            printf("%d  %s\n", p, i->command);
            p++;
        }
    }
}

void procesar_entrada(char *command, tListCom *listcom, tListFic *listafic, tListMem *listamemoria, BackgroundProcessList *processlist, int *exit, int storeCommand);

void commandN(char *command, tListCom *list, tListFic *listfic, tListMem *listamemoria, BackgroundProcessList *LP, int *exit) {
    int p = 0;
    tPosCom i;

    // Comprobamos que el numero de comando introducido es válido
    if (!es_numero(command)) {
        printf("El numero de comando introducido no es válido: %s\n", command);
        return;
    }

    int commandNumber = atoi(command);

    // Comprobamos que el número de comando introducido esta en la lista de commandos
    if (commandNumber < 0 || commandNumber >= total_comandos(*list)) {
        printf("El número de comando introducido no existe: %d\n", commandNumber);
        return;
    }
    // Se recorre la lista en busca del comando indicado
    for (i = *list; i->next != NULL && p < commandNumber; i = i->next) {
        p++;
    }
    // Se crea una copia de i-> command para evitar modificar el guardado en la lista, y se pasa a procesar_entrada, pero diciendole que no lo guarde en la lista de comandos
    char *commandCopy = strdup(i->command);
    if (commandCopy != NULL) {
        int storeCommand = 0;
        procesar_entrada(commandCopy, list, listfic, listamemoria, LP, exit, storeCommand);
        free(commandCopy);
    } else {
        perror("No se pudo ejecutar el comando solicitado");
    }
}

void Cmd_open(char *tr[], tListFic listaficheros) {
    int i, df, mode = 0;

    if (tr[1] == NULL) {
        listar_ficheros_abiertos(&listaficheros);
        return;
    }
    for (i = 2; tr[i] != NULL; i++) {
        if (!strcmp(tr[i], "cr")) mode |= O_CREAT;
        else if (!strcmp(tr[i], "ex")) mode |= O_EXCL;
        else if (!strcmp(tr[i], "ro")) mode |= O_RDONLY;
        else if (!strcmp(tr[i], "wo")) mode |= O_WRONLY;
        else if (!strcmp(tr[i], "rw")) mode |= O_RDWR;
        else if (!strcmp(tr[i], "ap")) mode |= O_APPEND;
        else if (!strcmp(tr[i], "tr")) mode |= O_TRUNC;
        else break;
    }
    if ((df = open(tr[1], mode, 0777)) == -1) {
        perror("Imposible abrir fichero");
        return;
    }

    AddFic(&listaficheros, df, mode, tr[1]);
    printf("Anadida entrada a la tabla ficheros abiertos\n");
}

void Cmd_close(char *command, tListFic list) {
    int df;

    if (command == NULL || (df = atoi(command)) < 0) {
        listar_ficheros_abiertos(&list);
        return;
    }
    if (close(df) == -1) {
        perror("Imposible cerrar descriptor\n");
    } else {
        RemoveFic(&list, df);
        printf("Descriptor %d cerrado y eliminado de la lista de ficheros abiertos\n", df);
    }
}

void Cmd_dup(char *command, tListFic list) {
    int df, duplicado;
    char aux[MAXLEN], *p;

    if (command == NULL || (df = atoi(command)) < 0) {
        listar_ficheros_abiertos(&list);
        return;
    }
    p = nombre_fichero_descriptor(list, df);

    if ((duplicado = dup(df)) == -1) {
        perror("Error duplicando el fichero\n");
        return;
    }
    snprintf(aux, sizeof(aux), "%s", p);
    AddFic(&list, duplicado, fcntl(duplicado, F_GETFL), aux);
    printf("Duplicado fichero %d a %d (%s)\n", df, duplicado, aux);
    free(p);
}

void listopen(tListFic *list) {
    listar_ficheros_abiertos(list);
}

void infosys() {
    struct utsname data;
    uname(&data);
    printf("%s %s %s %s %s\n", data.sysname, data.nodename, data.release, data.version, data.machine);
}

void help(char *command){
    if (command == NULL){
        printf("authors [-l|-n]\n"
               "pid [-p]\n"
               "chdir [dir]\n"
               "date\n"
               "time\n"
               "hist [-c|-N]\n"
               "comand N\n"
               "open [file] mode\n"
               "close [df]\n"
               "dup [df]\n"
               "listopen\n"
               "infosys\n"
               "help [cmd]\n"
               "quit\n"
               "exit\n"
               "bye\n"
               "create [-f] [name]\n"
               "stat [-long] [-link] [-acc] name1 name2\n"
               "list [-reca] [-recb] [-hid] [-link] [-acc] n1 n2\n"
               "delete [name1 name2 ..]\n"
               "deltree [name1 name2 ..]\n"
               "malloc [-free] [tam]\n"
               "shared [-free|-create|-delkey] cl [tam]\n"
               "mmap [-free] fich prm\n"
               "read fiche addr cont\n"
               "write [-o] fiche addr cont\n"
               "memdump addr cont\n"
               "memfill addr cont byte\n"
               "mem [-blocks|-funcs|-vars|-all|-pmap]\n"
               "recurse [n]\n"
               "uid [-get|-set] [-l] [id]\n"
               "showenv [-environ|-addr]\n"
               "showvar var\n"
               "changevar [-a|-e|-p] var valor\n"
               "subsvar [-a|-e] var1 var2 valor\n"
               "fork\n"
               "exec prog args\n"
               "jobs\n"
               "deljobs [-term][-sig]\n"
               "job [-fg] pid\n");
    }else if (!strcmp(command, "authors")){
        printf("authors [-l|-n]\t Muestra los logins y/o los nombres de los usuarios.\n");

    }else if (!strcmp(command, "pid")){
        printf("pid [-p]\t Muestra el pid del shell o el del padre.\n");

    }else if (!strcmp(command, "chdir")){
        printf("chdir [dir]\t Cambia o muestra el directorio actual o al que quieres acceder.\n");

    }else if (!strcmp(command, "date")){
        printf("date\t Muestra la fecha del sistema\n");

    }else if (!strcmp(command, "time")){
        printf("time\t Muestra la hora del sistema\n");

    }else if (!strcmp(command, "hist")){
        printf("hist[-c|-N]\t Muestra o borra el historial de comandos\n\t -N Muestra los N primeros.\n\t -c Borra el historial\n");

    }else if (!strcmp(command, "command")){
        printf("command [-N]\t Repite el comando numero N del historial.\n");

    }else if (!strcmp(command, "open")){
        printf("open [fich.] mode\t Abre el fichero con nombre [fich.] y lo añade a la lista de ficheros abiertos.\n\t mode : son los distintos modos de apertura.\n\t\t cr: O_CREAT\n\t\t ap: O_APPEND\n\t\t ex: O_EXCL\n\t\t ro: O_RDONLY\n\t\t rw: O_RDWR\n\t\t wo: O_WRNLY\n\t\t tr: O_TRUNC\n ");

    }else if (!strcmp(command, "close")){
        printf("close df\t Cierra el descriptor df y lo elimina de la lista de ficheros abiertos\n");

    }else if (!strcmp(command, "dup")){
        printf("dup df\t Duplica el descriptor de fichero df y añade una nueva entrada a la lista de ficheros abiertos\n ");

    }else if (!strcmp(command, "listopen")){
        printf("listopen[n]\n Lista los ficheros abiertos(al menos n) del shell\n");

    }else if (!strcmp(command, "infosys")){
        printf("infosys\t Muestra informacion de la maquina donde corre el shell\n");

    }else if (!strcmp(command, "help")){
        printf("help [cmd]\t Muestra informacion sobre el comando cmd\n");

    }else if (!strcmp(command, "quit")){
        printf("quit\t Termina la ejecucion del shell\n");

    }else if (!strcmp(command, "exit")){
        printf("exit\t Termina la ejecucion del shell\n");

    }else if (!strcmp(command, "bye")){
        printf("bye\t Termina la ejecucion del shell\n");

    }else if (!strcmp(command, "create")){
        printf("create [-f] [name]\t Crea un directorio o un fichero\n");

    }else if (!strcmp(command, "stat")){
        printf("stat [-long] [-link] [-acc] name1 name2 ..          lista ficheros;\n\t -long: listado largo\n\t -acc: acesstime \n\t -link: si es enlace simbolico, el path contenido\n");

    }else if (!strcmp(command, "list")){
        printf("list [-reca] [-recb] [-hid] [-link] [-acc] n1 n2..           lista contenidos de directorio\n\t -hid: incluye los ficheros ocultos.\n\t -recb: recursivo (antes)\n\t -reca: recursivo (despues)\n\t resto de parametros como stat\n ");

    }else if (!strcmp(command, "delete")){
        printf("delete [name1 name2 ..]\t Borra ficheros o directorios vacios\n");

    }else if (!strcmp(command, "deltree")){
        printf("deltree [name1 name2 ..]\t Borra ficheros o diretorios no vacios recursivamente\n");

    }else if (!strcmp((command), "malloc")){
        printf("malloc [-free] [tam]\tasigna un bloque memoria de tamano tam con malloc\n"
               "\t-free: desasigna un bloque de memoria de tamano tam asignado con malloc\n");

    }else if (!strcmp((command), "shared")){
        printf("shared [-free|-create|-delkey] cl [tam]\tasigna memoria compartida con clave cl en el programa\n"
               "\t-create cl tam: asigna (creando) el bloque de memoria compartida de clave cl y tamano tam\n"
               "\t-free cl: desmapea el bloque de memoria compartida de clave cl\n"
               "\t-delkey clelimina del sistema (sin desmapear) la clave de memoria cl\n");

    }else if (!strcmp((command), "mmap")){
        printf("mmap [-free] fich prm\tmapea el fichero fich con permisos prm\n"
               "\t-free fich: desmapea el ficherofich\n");

    }else if (!strcmp((command), "read")){
        printf("read fiche addr cont \tLee cont bytes desde fich a la direccion addr\n");

    }else if (!strcmp((command), "write")){
        printf("write [-o] fiche addr cont \tEscribe cont bytes desde la direccion addr a fich (-o sobreescribe)\n");

    }else if (!strcmp((command), "memdump")){
        printf("memdump addr cont \tVuelca en pantallas los contenidos (cont bytes) de la posicion de memoria addr\n");

    }else if (!strcmp((command), "memfill")){
        printf("memfill addr cont byte \tLlena la memoria a partir de addr con byte\n");

    }else if (!strcmp((command), "mem")){
        printf("mem [-blocks|-funcs|-vars|-all|-pmap] ..\tMuestra muestra detalles de la memoria del proceso\n"
               "\t\t-blocks: los bloques de memoria asignados\n"
               "\t\t-funcs: las direcciones de las funciones\n"
               "\t\t-vars: las direcciones de las variables\n"
               "\t\t:-all: todo\n"
               "\t\t-pmap: muestra la salida del comando pmap(o similar)\n");

    }else if (!strcmp((command),"recurse")){
        printf("recurse [n]\tInvoca a la funcion recursiva n veces\n");

    }else if (!strcmp((command), "uid")) {
        printf("uid [-get|-set] [-l] [id] \t Accede a las credenciales del proceso que ejecuta el shell\n"
               "\t-get: muestra las credenciales\n"
               "\t-set id: establece la credencial al valor numerico id\n"
               "\t-set -l id: establece la credencial a login id\n");

    }else if (!strcmp((command), "showenv")) {
        printf("showenv [-environ|-addr] \t Muestra el entorno del proceso\n"
               "\t-environ: accede usando environ (en lugar del tercer arg de main)\n"
               "\t-addr: muestra el valor y donde se almacenan environ y el 3er arg main\n");

    }else if (!strcmp((command), "showvar")) {
        printf("showvar var\tMuestra el valor y las direcciones de la variable de entorno var\n");

    }else if (!strcmp((command), "changevar")) {
        printf("changevar [-a|-e|-p] var valor\tCambia el valor de una variable de entorno\n"
               "\t-a: accede por el tercer arg de main\n"
               "\t-e: accede mediante environ\n"
               "\t-p: accede mediante putenv\n");

    }else if (!strcmp((command), "subsvar")) {
        printf("subsvar [-a|-e] var1 var2 valor\tSustituye la variable de entorno var1\n"
               "\tcon var2=valor\n"
               "\t-a: accede por el tercer arg de main\n"
               "\t-e: accede mediante environ\n");

    }else if (!strcmp((command), "fork")) {
        printf("fork \tEl shell hace fork y queda en espera a que su hijo termine\n");

    }else if (!strcmp((command), "exec")) {
        printf("exec prog args\tEjecuta, sin crear proceso,prog con argumentos\n");

    }else if (!strcmp((command), "jobs")) {
        printf("jobs \tLista los procesos en segundo plano\n");

    }else if (!strcmp((command), "deljobs")) {
        printf("deljobs [-term][-sig]\tElimina los procesos de la lista procesos en sp\n"
               "\t-term: los terminados\n"
               "\t-sig: los terminados por senal\n");

    }else if (!strcmp((command), "job")) {
        printf("job [-fg] pid\tMuestra informacion del proceso pid.\n"
               "\t\t-fg: lo pasa a primer plano\n");

    }
    else{
        printf("comando no reconocido.\n");

    }
}

void imprimirPrompt() {
    printf("->");
}

void leerEntrada(char *command){
    fgets(command, MAXLEN, stdin);
}
