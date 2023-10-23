//Autor 1: José Manuel Ferreiro López   login: jose.manuel.ferreiro@udc.es
//Autor 2: Alvaro Mendez Linares    login: alvaro.mendez.linares@udc.es

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <sys/utsname.h>
#include <fcntl.h>
#include "lista_comandos.h"
#include "lista_ficheros.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <pwd.h>
#include <grp.h>
#include <limits.h>
#include <errno.h>
#include <dirent.h>
#define MAXLEN 100

char *get_permission_string(mode_t mode) {
    static char perms[11];

    perms[0] = (S_ISDIR(mode)) ? 'd' : '-';
    perms[1] = (mode & S_IRUSR) ? 'r' : '-';
    perms[2] = (mode & S_IWUSR) ? 'w' : '-';
    perms[3] = (mode & S_IXUSR) ? 'x' : '-';
    perms[4] = (mode & S_IRGRP) ? 'r' : '-';
    perms[5] = (mode & S_IWGRP) ? 'w' : '-';
    perms[6] = (mode & S_IXGRP) ? 'x' : '-';
    perms[7] = (mode & S_IROTH) ? 'r' : '-';
    perms[8] = (mode & S_IWOTH) ? 'w' : '-';
    perms[9] = (mode & S_IXOTH) ? 'x' : '-';
    perms[10] = '\0';

    return perms;
}

void list_directory_contents(char *directory, int show_hidden, int long_format, int link_format, int access_time) {
    DIR *dir;
    struct dirent *entry;
    struct stat file_stat;

    if ((dir = opendir(directory)) == NULL) {
        perror("opendir");
        return;
    }

    while ((entry = readdir(dir)) != NULL) {
        if (show_hidden || (entry->d_name[0] != '.')) {
            char file_path[PATH_MAX];
            snprintf(file_path, sizeof(file_path), "%s/%s", directory, entry->d_name);

            if (stat(file_path, &file_stat) == -1) {
                perror("stat");
                continue;
            }

            if (long_format) {
                char permission_string[20];
                strftime(permission_string, sizeof(permission_string), "%Y/%m/%d-%H:%M", localtime(&file_stat.st_mtime));

                printf("%s %lu (%lu)%s%s %s %ld %s\n",
                       permission_string,
                       (unsigned long)file_stat.st_nlink,
                       file_stat.st_ino,
                       getpwuid(file_stat.st_uid)->pw_name,
                       getgrgid(file_stat.st_gid)->gr_name,
                       get_permission_string(file_stat.st_mode),
                       file_stat.st_size,
                       entry->d_name);
            } else if (access_time) {
                printf("%s  %s\n", ctime(&file_stat.st_atime), entry->d_name);
            } else if (link_format) {
                if (S_ISREG(file_stat.st_mode)) {
                    printf("%ld  %s\n", file_stat.st_size, entry->d_name);
                } else {
                    printf("N/A  %s (not a regular file)\n", entry->d_name);
                }
            } else {
                printf("%s\n", entry->d_name);
            }
        }
    }

    closedir(dir);
}

void list_recursive_before(char *directory, int show_hidden, int long_format, int link_format, int access_time) {
    DIR *dir;
    struct dirent *entry;

    if ((dir = opendir(directory)) == NULL) {
        perror("opendir");
        return;
    }

    printf("****%s****\n", directory); //xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx

    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type == DT_DIR) {
            char file_path[PATH_MAX];
            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
                continue;
            }
            snprintf(file_path, sizeof(file_path), "%s/%s", directory, entry->d_name);
            list_recursive_before(file_path, show_hidden, long_format, link_format, access_time);
        }
    }

    closedir(dir);
    list_directory_contents(directory, show_hidden, long_format, link_format, access_time);
}

void list_recursive_after(char *directory, int show_hidden, int long_format, int link_format, int access_time) {
    printf("****%s****\n", directory);

    list_directory_contents(directory, show_hidden, long_format, link_format, access_time);
    DIR *dir;
    struct dirent *entry;

    if ((dir = opendir(directory)) == NULL) {
        perror("opendir");
        return;
    }

    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type == DT_DIR) {
            char file_path[PATH_MAX];
            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
                continue;
            }
            snprintf(file_path, sizeof(file_path), "%s/%s", directory, entry->d_name);
            list_recursive_after(file_path, show_hidden, long_format, link_format, access_time);
        }
    }

    closedir(dir);
}

int is_hidden(const char *name) {
    return name[0] == '.' && strcmp(name, "..") != 0;
}

void create(char *command, char *name) {
    char temp[MAXLEN];

    if (command == NULL) {
        if (getcwd(temp, MAXLEN) != NULL) {
            printf("Directorio actual: %s\n", temp);
        } else {
            perror("create"); //xxxxxxxxxxxxxxxxxxxxxxxxxxxxx
        }
    } else if (command != NULL && name == NULL) {
        int result = mkdir(command, 0700);
        if (result == -1) {
            perror("create"); //xxxxxxxxxxxxxxxxxxxxxxxxxxxxx
        }
    } else if (!strcmp(command, "-f") && name != NULL) {
        int fd = creat(name, 0666);
        if (fd == -1) {
            perror("create"); //xxxxxxxxxxxxxxxxxxxxxxxxxxxxx
        } else {
            close(fd);
        }
    } else {
        printf("Invalid usage of the create command.\n");
    }
}


void stat_fun(char *command, char *names[]) {
    int long_format = 0, access_time = 0, symbolic_link = 0;
    char temp[MAXLEN];
    if (command == NULL) {
        if (getcwd(temp, MAXLEN) != NULL) {
            printf("Directorio actual: %s\n", temp);
        } else {
            perror("stat");
        }
        return;
    }

    if (strcmp(command, "-long") == 0) {
        long_format = 1;
    } else if (strcmp(command, "-acc") == 0) {
        access_time = 1;
    } else if (strcmp(command, "-link") == 0) {
        symbolic_link = 1;
    } else {
        char *aux1 = names[0];
        char *aux2;
        names[0] = command;
        for (int i = 0; names[i] != NULL; i++){
            aux2 = names[i+1];
            names[i+1] = aux1;
            aux1 = aux2;
        }
    }

    for (int i = 0; names[i] != NULL; i++) {
        struct stat file_stat;

        if (stat(names[i], &file_stat) == -1) {
            perror("stat");
            continue;  // Move to the next file on error
        }

        char permission_string[20];
        strftime(permission_string, sizeof(permission_string), "%Y/%m/%d-%H:%M", localtime(&file_stat.st_mtime));
        if (long_format) {
            struct passwd *pwd = getpwuid(file_stat.st_uid);
            struct group *grp = getgrgid(file_stat.st_gid);

            printf("%s %lu (%lu)%s%s %s %ld %s\n",
                   permission_string,
                   (unsigned long)file_stat.st_nlink,
                   file_stat.st_ino,
                   pwd->pw_name,
                   grp->gr_name,
                   get_permission_string(file_stat.st_mode),
                   file_stat.st_size,
                   names[i]);
        } else if (access_time) {
            printf("%s  %s\n", permission_string, names[i]);
        } else if (symbolic_link) {
            printf("%ld  %s\n", file_stat.st_size, names[i]);
        }
        else {
            printf("%ld  %s\n", file_stat.st_size, names[i]);
        }
    }
}

void list_fun(char *command, char *names[]) {
    char *commands[MAXLEN];
    char *filtered_names[MAXLEN];
    int recursive_after = 0;
    int recursive_before = 0;
    int show_hidden = 0;
    int long_format = 0;
    int link_format = 0;
    int access_time = 0;

    // Initialize arrays
    for (int i = 0; i < MAXLEN; i++) {
        commands[i] = NULL;
        filtered_names[i] = NULL;
    }

    // Separate commands and names
    int cmd_index = 0;
    int names_index = 0;

    // If command is not NULL, add it to the commands array
    if (command != NULL) {
        if (names[0] == NULL) {
            filtered_names[names_index++] = command; // Treat it as a folder name
        } else {
            commands[cmd_index++] = command;
        }
    }

    // Filter and separate names
    for (int i = 0; names[i] != NULL; i++) {
        if (!strcmp(names[i], "-reca") || !strcmp(names[i], "-recb") || !strcmp(names[i], "-hid") ||
            !strcmp(names[i], "-long") || !strcmp(names[i], "-link") || !strcmp(names[i], "-acc")) {
            commands[cmd_index++] = names[i];
        } else {
            filtered_names[names_index++] = names[i];
        }
    }

    // Handle commands and filtered names
    for (int i = 0; commands[i] != NULL; i++) {
        // Handle commands
        if (strcmp(commands[i], "-reca") == 0) {
            recursive_after = 1;
        } else if (strcmp(commands[i], "-recb") == 0) {
            recursive_before = 1;
        } else if (strcmp(commands[i], "-hid") == 0) {
            show_hidden = 1;
        } else if (strcmp(commands[i], "-long") == 0) {
            long_format = 1;
        } else if (strcmp(commands[i], "-link") == 0) {
            link_format = 1;
        } else if (strcmp(commands[i], "-acc") == 0) {
            access_time = 1;
        }
    }

    // Handle filtered names
    for (int i = 0; filtered_names[i] != NULL; i++) {
        char *current_directory = filtered_names[i];

        if (recursive_before) {
            list_recursive_before(current_directory, show_hidden, long_format, link_format, access_time);
        } else if (recursive_after) {
            list_recursive_after(current_directory, show_hidden, long_format, link_format, access_time);
        } else {
            list_directory_contents(current_directory, show_hidden, long_format, link_format, access_time);
        }
    }
}

void delete(const char *path) {
    if (remove(path) == 0) {
        printf("The file or directory %s has been deleted successfully\n", path);
    } else {
        if (errno == EACCES) {
            perror("ERROR");
        } else if (errno == ENOENT) {
            perror("ERROR");
        } else if (errno == ENOTDIR) {
            if (rmdir(path) == 0) {
                printf("The directory %s has been deleted successfully\n", path);
            } else {
                perror("ERROR");
            }
        } else {
            printf("Error deleting the file or directory %s\n", path);
        }
    }
}

void deltree (){

}


char *NombreFicheroDescriptor(tListFic list, int df) {
    tPosFic current = list;

    while (current != NULL) {
        if (current->descriptor == df) {
            return strdup(current->nombre);
        }
        current = current->next;
    }

    char *error_message = strdup("Descriptor no encontrado");
    return error_message;
}

void ListarFicherosAbiertos(tListFic *list) {
    tPosFic current = *list;

    while (current != NULL) {
        char *mode_str = "";

        int mode = fcntl(current->descriptor, F_GETFL);
        if (mode == -1) {
            perror("Error fcntl:");
            mode_str = "Desconocido";
        } else {
            if (mode & O_RDONLY) mode_str = "O_RDONLY";
            else if (mode & O_EXCL) mode_str = "O_EXCL";
            else if (mode & O_WRONLY) mode_str = "O_WRONLY";
            else if (mode & O_RDWR) mode_str = "O_RDWR";
            else if (mode & O_APPEND) mode_str = "O_APPEND";
            else if (mode & O_TRUNC) mode_str = "O_TRUNC";
            else if (mode & O_CREAT) mode_str = "O_CREAT";
            else mode_str = "Desconocido";
        }
        printf("descriptor: %d -> %s %s\n", current->descriptor, current->nombre, mode_str);
        current = current->next;
    }
}

int TrocearCadena(char * cadena, char * trozos[]){
    int i=1;
    if ((trozos[0]=strtok(cadena," \n\t"))==NULL)
        return 0;
    while ((trozos[i]=strtok(NULL," \n\t"))!=NULL)
        i++;
    return i;
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

void time_fun(){
    time_t t = time(NULL);
    struct tm tm = *localtime(&t);
    printf("%02d:%02d:%02d\n", tm.tm_hour, tm.tm_min, tm.tm_sec);
}

void hist(char *command, tListCom *list){
    if (command == NULL){
        int p = 0;
        for(tPosCom i = *list; i != NULL; i = i->next){
            printf("%d  %s\n", p, i->command);
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
void procesarEntrada(char *command, tListCom *listcom, tListFic *listafic, int *exit);

void commandN(char *command, tListCom *list, tListFic *listfic, int *exit){
    int p = 1;
    tPosCom i;
    for(i = *list; i->next != NULL && p <= atoi(command); i = i->next){
        p++;
    }
    procesarEntrada(i->command, list, listfic, exit);
}

void Cmd_open(char *tr[], tListFic listaficheros) {
    int i, df, mode = 0;

    if (tr[1] == NULL) {
        ListarFicherosAbiertos(&listaficheros);
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
        perror("Imposible abrir fichero\n");
        return;
    }

    AñadirAFicherosAbiertos(&listaficheros, df, mode, tr[1]);
    printf("Anadida entrada a la tabla ficheros abiertos\n");
}

void Cmd_close(char *command, tListFic list) {
    int df;

    if (command == NULL || (df = atoi(command)) < 0) {
        ListarFicherosAbiertos(&list);
        return;
    }
    if (close(df) == -1) {
        perror("Imposible cerrar descriptor\n");
    } else {
        RemoveFic(&list, df);
        printf("Descriptor %d cerrado y eliminado de la lista de ficheros abiertos.\n", df);
    }
}

void Cmd_dup(char *command, tListFic list) {
    int df, duplicado;
    char aux[MAXLEN], *p;

    if (command == NULL || (df = atoi(command)) < 0) {
        ListarFicherosAbiertos(&list);
        return;
    }
    p = NombreFicheroDescriptor(list, df);

    if ((duplicado = dup(df)) == -1) {
        perror("Error duplicando el fichero: \n");
        return;
    }
    snprintf(aux, sizeof(aux), "%s", p);
    AñadirAFicherosAbiertos(&list, duplicado, fcntl(duplicado, F_GETFL), aux);
    printf("Duplicado fichero %d a %d (%s)\n", df, duplicado, aux);
    free(p);
}

void listopen(tListFic *list) {
    ListarFicherosAbiertos(list);
}

void infosys(){
    struct utsname data;
    uname(&data);
    printf("%s %s %s %s %s\n", data.sysname, data.nodename, data.release, data.version, data.machine);
}

void help(char *command){
    if (command == NULL){
        printf("authors [-l|-n]\n");
        printf("pid [-p]\n");
        printf("chdir [dir]\n");
        printf("date\n");
        printf("time\n");
        printf("hist [-c|-N]");
        printf("comand N\n");
        printf("open [file] mode\n");
        printf("close [df]\n");
        printf("dup [df]");
        printf("listopen\n");
        printf("infosys\n");
        printf("help [cmd]\n");
        printf("quit\n");
        printf("exit\n");
        printf("bye\n");
        printf("create [-f] [name]\n");
        printf("stat [-long] [-link] [-acc] name1 name2\n");
        printf("list [-reca] [-recb] [-hid] [-link] [-acc] n1 n2\n");
        printf("delete [name1 name2 ..]\n");
        printf("deltree name1 name2 ..]\n");
    }else if (!strcmp(command, "authors")){
        printf("authors [-l|-n]\t Muestra los logins y/o los nombres de los usuarios.\n");

    }else if (!strcmp(command, "pid")){
        printf("pid [-p]\t Muestra el pid del shell o el del padre.\n");

    }else if (!strcmp(command, "chdir")){
        printf("chdir [dir]\t Cambia o muestra el directorio actual o al que quieres acceder.\n");

    }else if (!strcmp(command, "date")){
        printf("date\t Muestra la fecha del sistema");

    }else if (!strcmp(command, "time")){
        printf("time\t Muestra la hora del sistema");

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
        printf("deltree [name1 name2 ..]\t Borra ficheros o diretorios no vacios recursivamente\np ");

    }else{
        printf("comando no reconocido.\n");

    }
}

void imprimirPrompt(){
    printf("->");
}

void leerEntrada(char *command){
    fgets(command, MAXLEN, stdin);
}

void procesarEntrada(char *command, tListCom *listcom, tListFic *listafic, int *exit) {
    char *trozos[4];

    InsertCommand(listcom, &command);
    int numt = TrocearCadena(command, trozos);

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
            commandN(trozos[1], listcom, listafic, exit);

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
            stat_fun(trozos[1], trozos + 2);

        }else  if (!strcmp(trozos[0], "list")){
            list_fun(trozos[1], trozos + 2);

        } else  if (!strcmp(trozos[0], "delete")){
            delete(trozos[1]);

        } else  if (!strcmp(trozos[0], "deltree")){
            deltree(trozos[1]);

        } else {
            printf("comando no reconocido\n");
        }
    }
}

int main() {
    char command[MAXLEN];
    int exit = 0;

    tListCom list;
    tListFic listaficheros;
    CreateListCom(&list);
    CreateListFic(&listaficheros);

    AñadirAFicherosAbiertos(&listaficheros, 0, O_RDWR, "entrada estandar");
    AñadirAFicherosAbiertos(&listaficheros, 1, O_RDWR, "salida estandar");
    AñadirAFicherosAbiertos(&listaficheros, 2, O_RDWR, "error estandar");

    while (exit == 0){
        imprimirPrompt();
        leerEntrada(command);
        procesarEntrada(command, &list, &listaficheros, &exit);
    }
    ClearListFic(&listaficheros);
    ClearListCom(&list);
    return 0;
}