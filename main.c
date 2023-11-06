//Autor 1: José Manuel Ferreiro López   login: jose.manuel.ferreiro@udc.es
//Autor 2: Alvaro Mendez Linares    login: alvaro.mendez.linares@udc.es

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <sys/utsname.h>
#include <fcntl.h>
#include "lista_comandos.h"
#include "lista_ficheros.h"

#include <sys/stat.h>
#include <pwd.h>
#include <grp.h>
#include <limits.h>
#include <errno.h>
#include <dirent.h>
#include <ctype.h>
#include <sys/wait.h>

#define MAXLEN 200
#define TROZOS 15  //numere de trozos que se pueden almacenar en la array trozos
#define TAMANO 2048

void Recursiva (int n) {
    char automatico[TAMANO];
    static char estatico[TAMANO];

    printf ("parametro:%3d(%p) array %p, arr estatico %p\n",n,&n,automatico, estatico);

    if (n>0)
        Recursiva(n-1);
}

void Do_MemPmap (void) { /*sin argumentos*/
    pid_t pid;       /*hace el pmap (o equivalente) del proceso actual*/
    char elpid[32];
    char *argv[4]={"pmap",elpid,NULL};

    sprintf (elpid,"%d", (int) getpid());
    if ((pid=fork())==-1){
        perror ("Imposible crear proceso");
        return;
    }
    if (pid==0){ /*proceso hijo*/
        if (execvp(argv[0],argv)==-1)
            perror("cannot execute pmap (linux, solaris)");

        argv[0]="vmmap"; argv[1]="-interleave"; argv[2]=elpid;argv[3]=NULL;
        if (execvp(argv[0],argv)==-1) /*probamos vmmap Mac-OS*/
            perror("cannot execute vmmap (Mac-OS)");

        argv[0]="procstat"; argv[1]="vm"; argv[2]=elpid; argv[3]=NULL;
        if (execvp(argv[0],argv)==-1)/*No hay pmap, probamos procstat FreeBSD */
            perror("cannot execute procstat (FreeBSD)");

        argv[0]="procmap",argv[1]=elpid;argv[2]=NULL;
        if (execvp(argv[0],argv)==-1)  /*probamos procmap OpenBSD*/
            perror("cannot execute procmap (OpenBSD)");

        exit(1);
    }
    waitpid (pid,NULL,0);
}

void malloc_fun (size_t size){
    malloc(size);

}

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

char LetraTF (mode_t m)
{
    switch (m&S_IFMT) { /*and bit a bit con los bits de formato,0170000 */
        case S_IFSOCK: return 's'; /*socket */
        case S_IFLNK: return 'l'; /*symbolic link*/
        case S_IFREG: return '-'; /* fichero normal*/
        case S_IFBLK: return 'b'; /*block device*/
        case S_IFDIR: return 'd'; /*directorio */
        case S_IFCHR: return 'c'; /*char device*/
        case S_IFIFO: return 'p'; /*pipe*/
        default: return '?'; /*desconocido, no deberia aparecer*/
    }
}

char *ConvierteModo2 (mode_t m)
{
    static char permisos[12];
    strcpy (permisos,"---------- ");

    permisos[0]=LetraTF(m);
    if (m&S_IRUSR) permisos[1]='r';    /*propietario*/
    if (m&S_IWUSR) permisos[2]='w';
    if (m&S_IXUSR) permisos[3]='x';
    if (m&S_IRGRP) permisos[4]='r';    /*grupo*/
    if (m&S_IWGRP) permisos[5]='w';
    if (m&S_IXGRP) permisos[6]='x';
    if (m&S_IROTH) permisos[7]='r';    /*resto*/
    if (m&S_IWOTH) permisos[8]='w';
    if (m&S_IXOTH) permisos[9]='x';
    if (m&S_ISUID) permisos[3]='s';    /*setuid, setgid y stickybit*/
    if (m&S_ISGID) permisos[6]='s';
    if (m&S_ISVTX) permisos[9]='t';

    return permisos;
}

void auxList_fun(char *directory, int show_hidden, int long_format, int link_format, int access_time) {
    DIR *dir;
    struct dirent *first;
    struct stat stats;

    if ((dir = opendir(directory)) == NULL) {
        perror("opendir");
        return;
    }
    printf("***********%s\n",directory);

    while ((first = readdir(dir)) != NULL) {
        //ESTE IF ES PARA -long
        //Si nos direon la opción -hid, las bucamos para mostrarlas
        if (show_hidden || (first->d_name[0] != '.')) {
            char path[PATH_MAX];
            snprintf(path, sizeof(path), "%s/%s", directory, first->d_name);
            //Se obtiene el path de los links simbólicos
            char target_path[MAXLEN];
            ssize_t bytesRead = readlink(directory, target_path, sizeof(target_path) - 1);
            if (link_format) {
                if (bytesRead != -1) {
                    target_path[bytesRead] = '\0';
                } else {
                    link_format = 0;
                }
            }

            if (stat(path, &stats) == -1) {
                perror("No se pudo acceder al directorio");
                continue;
            }
            char time_string[20];
            strftime(time_string, sizeof(time_string), "%Y/%m/%d-%H:%M", localtime(&stats.st_mtime));

            if (long_format) {

                //Obtenemos las stats del directorio
                struct stat link_stat;
                if (lstat(directory, &link_stat) == -1) {
                    perror("No se pudo aceder al directorio");
                }
                //Salida para -long (las variables hacen que segun se acompañe de -acc o -link o ambas, la salida se adapte)
                ino_t inode = link_stat.st_ino;
                printf("%s %lu (%lu) %s%s %s %ld %s%s%s\n",
                       time_string,
                       (unsigned long) stats.st_nlink,
                       (unsigned long) inode,
                       getpwuid(stats.st_uid)->pw_name,
                       getgrgid(stats.st_gid)->gr_name,
                       ConvierteModo2(stats.st_mode),
                       stats.st_size,
                       first->d_name,
                       link_format ? "->" : "",
                       link_format ? target_path : "");
            } else if (access_time) {
                printf("%s  %s\n", time_string, first->d_name);
            } else if (link_format) {
                if (S_ISREG(stats.st_mode)) {
                    printf("%ld  %s %s %s\n", stats.st_size, first->d_name, link_format? "->" : "", link_format? target_path : "");
                } else {
                    printf("N/A  %s (not a regular file)\n", first->d_name);
                }
            } else {
                printf("%8ld %s\n", stats.st_size, first->d_name);
            }
        }
    }
    closedir(dir);
}

void recb(char *directory, int show_hidden, int long_format, int link_format, int access_time) {
    DIR *dir;
    struct dirent *first;

    if ((dir = opendir(directory)) == NULL) {
        perror("opendir");
        return;
    }

    while ((first = readdir(dir)) != NULL) {
        /*Comprobamos si first es un directorio, de ser así obtenemos los subdirectorios y hacemos
         * la llamada recursiva sobre ellos */
        if (first->d_type == DT_DIR) {
            char path[PATH_MAX];
            //Nos saltamos . y .. para evitar un bucle infinito
            if (strcmp(first->d_name, ".") == 0 || strcmp(first->d_name, "..") == 0) {
                continue;
            }
            //Contruimos path (el camino al siguiente directorio)
            snprintf(path, sizeof(path), "%s/%s", directory, first->d_name);
            recb(path, show_hidden, long_format, link_format, access_time);
        }
    }
    closedir(dir);
    //Listamos los contenidos
    auxList_fun(directory, show_hidden, long_format, link_format, access_time);
}

void reca(char *directory, int show_hidden, int long_format, int link_format, int access_time) {
    //Listamos los contenidos
    auxList_fun(directory, show_hidden, long_format, link_format, access_time);
    DIR *dir;
    struct dirent *first;

    if ((dir = opendir(directory)) == NULL) {
        perror("opendir");
        return;
    }

    while ((first = readdir(dir)) != NULL) {
        /*Comprobamos si first es un directorio, de ser así obtenemos los subdirectorios y hacemos
         * la llamada recursiva sobre ellos */
        if (first->d_type == DT_DIR) {
            char path[PATH_MAX];
            //Nos saltamos . y .. para evitar un bucle infinito
            if (strcmp(first->d_name, ".") == 0 || strcmp(first->d_name, "..") == 0) {
                continue;
            }
            //Contruimos path (el camino al siguiente directorio)
            snprintf(path, sizeof(path), "%s/%s", directory, first->d_name);
            reca(path, show_hidden, long_format, link_format, access_time);
        }
    }
    closedir(dir);
}

void create(char *command, char *name) {
    char temp[MAXLEN];

    if (command == NULL) {
        if (getcwd(temp, MAXLEN) != NULL) {
            printf("Directorio actual: %s\n", temp);
        } else {
            perror("No se pudo encontrar el directorio actual");
        }
        // Si no hay nombre pero si opción significa que la opción es el nombre
    } else if (command != NULL && name == NULL) {
        int result = mkdir(command, 0700);
        if (result == -1) {
            perror("No se pudo crear el directorio");
        }
        // Si la opción el 2-f", crea un fichero con el nombre proporcionado
    } else if (!strcmp(command, "-f") && name != NULL) {
        int fd = creat(name, 0666);
        if (fd == -1) {
            perror("No se pudo crear el fichero");
        } else {
            close(fd);
        }
    }
}

void stat_fun(char *tr[]) {
    char *commands[3] ={NULL, NULL, NULL};
    char *filtered_names[20];
    char temp[MAXLEN];
    int long_format = 0;
    int link_format = 0;
    int access_time = 0;
    if (tr[0] == NULL) {
        if (getcwd(temp, MAXLEN) != NULL) {
            printf("Directorio actual: %s\n", temp);
        } else {
            perror("No se pudo encontrar el directorio actual");
        }
        return;
    }

    //Inicializamos filtered_names a NULL
    for (int i = 0; i < 20; i++) {
        filtered_names[i] = NULL;
    }

    int cmd_index = 0;
    int names_index = 0;

    /*Navegamos por la array tr[] y vamos guardando las opciones en commands[] hasta encontrar un item que no se
     * corresponde con una de las opciones, entonces se guardan el resto de items en la array filtered_names */
    for (int i = 0; tr[i] != NULL; i++) {
        if (!strcmp(tr[i], "-long") || !strcmp(tr[i], "-link") || !strcmp(tr[i], "-acc")) {
            commands[cmd_index++] = tr[i];
        } else {
            filtered_names[names_index++] = tr[i];
        }
    }

    for (int i = 0; commands[i] != NULL; i++) {
        if (strcmp(commands[i], "-long") == 0) {
            long_format = 1;
        } else if (strcmp(commands[i], "-link") == 0) {
            link_format = 1;
        } else if (strcmp(commands[i], "-acc") == 0) {
            access_time = 1;
        } else {
            break;
        }
    }

    for (int i = 0; filtered_names[i] != NULL; i++) {
        struct stat file_stat;
        if (stat(filtered_names[i], &file_stat) == -1) {
            perror("No se pudo encontrar el directorio o fichero");
            continue;
        }

        char time_string[20];
        /*Si -acc = 1 (se introdució la opción -acc), se guarda el tiempo de acceso, si acc = 0 (no se introdució
         * la opción -acc) se guarda el tiempo de modificación */
        strftime(time_string, sizeof(time_string), "%Y/%m/%d-%H:%M",
                 access_time ? localtime(&file_stat.st_atime) : localtime(&file_stat.st_mtime));
        if (long_format) {
            char target_path[MAXLEN];
            ssize_t bytesRead = readlink(filtered_names[i], target_path, sizeof(target_path) - 1);
            if (link_format) {
                if (bytesRead != -1) {
                    target_path[bytesRead] = '\0';
                } else {
                    link_format = 0;
                }
            }

            struct stat link_stat;
            if (lstat(filtered_names[i], &link_stat) == -1) {
                perror("No se pudo encontrar el directorio o fichero");
            }
            ino_t inode = link_stat.st_ino;
            printf("%s %lu (%lu) %s%s %s %ld %s%s%s\n",
                   time_string,
                   (unsigned long) file_stat.st_nlink,
                   (unsigned long) inode,
                   getpwuid(file_stat.st_uid)->pw_name,
                   getgrgid(file_stat.st_gid)->gr_name,
                   ConvierteModo2(file_stat.st_mode),
                   link_format ? (long) bytesRead : file_stat.st_size,
                   filtered_names[i],
                   link_format ? "->" : "",
                   link_format ? target_path : "");
        } else {
            printf("%ld  %s\n", file_stat.st_size, filtered_names[i]);
        }
    }
}

void list_fun(char *tr[]) {
    char *commands[6] ={NULL, NULL, NULL, NULL, NULL, NULL};
    char *filtered_names[20];
    char temp[MAXLEN];
    int recursive_after = 0;
    int recursive_before = 0;
    int show_hidden = 0;
    int long_format = 0;
    int link_format = 0;
    int access_time = 0;

    if (tr[0] == NULL) {
        if (getcwd(temp, MAXLEN) != NULL) {
            printf("Directorio actual: %s\n", temp);
        } else {
            perror("No se pudo encontrar el directorio actual");
        }
        return;
    }

    //Inicializamos filtered_names a NULL
    for (int i = 0; i < 20; i++) {
        filtered_names[i] = NULL;
    }

    int ops = 0;
    int names = 0;

    /*Navegamos por la array tr[] y vamos guardando las opciones en commands[] hasta encontrar un item que no se
     * corresponde con una de las opciones, entonces se guardan el resto de items en la array filtered_names */
    for (int i = 0; tr[i] != NULL; i++) {
        if (!strcmp(tr[i], "-reca") || !strcmp(tr[i], "-recb") || !strcmp(tr[i], "-hid") ||
            !strcmp(tr[i], "-long") || !strcmp(tr[i], "-link") || !strcmp(tr[i], "-acc")) {
            commands[ops++] = tr[i];
        } else {
            filtered_names[names++] = tr[i];
        }
    }

    for (int i = 0; commands[i] != NULL; i++) {
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

    for (int i = 0; filtered_names[i] != NULL; i++) {
        char *current_directory = filtered_names[i];

        if (recursive_before) {
            recb(current_directory, show_hidden, long_format, link_format, access_time);
        } else if (recursive_after) {
            reca(current_directory, show_hidden, long_format, link_format, access_time);
        } else {
            auxList_fun(current_directory, show_hidden, long_format, link_format, access_time);
        }
    }
}

void delete(char *paths[]) {
    char temp[MAXLEN];
    if (paths[0] == NULL) {
        if (getcwd(temp, MAXLEN) != NULL) {
            printf("Directorio actual: %s\n", temp);
        } else {
            perror("No se pudo encontrar el directorio actual");
        }
        return;
    }

    for (int i = 0; paths[i] != NULL; i++) {
        if (remove(paths[i]) == 0) {
            printf("El fichero o directorio %s ha sido eliminado\n", paths[i]);
        } else {
            if (errno == EACCES) {
                perror("ERROR");
            } else if (errno == ENOENT) {
                perror("ERROR");
            } else if (errno == ENOTDIR) {
                if (rmdir(paths[i]) == 0) {
                    printf("El directorio %s ha sido eliminado\n", paths[i]);
                } else {
                    perror("ERROR");
                }
            } else {
                printf("No se pudo eliminar %s\n", paths[i]);
            }
        }
    }
}

void deltree(char *paths[]) {
    struct stat statbuf;
    DIR *dir;
    struct dirent *entry;
    char temp[PATH_MAX];

    if (paths[0] == NULL) {
        if (getcwd(temp, PATH_MAX) != NULL) {
            printf("Directorio actual: %s\n", temp);
        } else {
            perror("No se pudo encontrar el directorio actual");
        }
        return;
    }


    for (int i = 0; paths[i] != NULL; i++) {

        if (lstat(paths[i], &statbuf) == -1) {
            perror("No se pudo encontrar el directorio o fichero");
            continue;
        }

        if ((dir = opendir(paths[i])) == NULL) {
            perror("No se pudo acceder al directorio");
            return;
        }

        while ((entry = readdir(dir)) != NULL) {
            char full_path[PATH_MAX];
            snprintf(full_path, sizeof(full_path), "%s/%s", paths[i], entry->d_name);

            if (lstat(full_path, &statbuf) == -1) {
                perror("No se pudo encontrar el directorio o fichero");
                continue;
            }

            if (S_ISDIR(statbuf.st_mode)) {
                if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
                    continue;
                }
                deltree((char *[]) {full_path, NULL });
            } else {
                if (unlink(full_path) != 0) {
                    perror("No se pudo eliminar el archivo");
                }
            }
        }
        closedir(dir);

        if (rmdir(paths[i]) != 0) {
            perror("No se pudo eliminar el directorio");
        }
    }
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

void procesar_entrada(char *command, tListCom *listcom, tListFic *listafic, int *exit, int storeCommand);

void commandN(char *command, tListCom *list, tListFic *listfic, int *exit) {
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
        procesar_entrada(commandCopy, list, listfic, exit, storeCommand);
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
        printf("deltree [name1 name2 ..]\n");
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
        printf("deltree [name1 name2 ..]\t Borra ficheros o diretorios no vacios recursivamente\n");

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

void procesar_entrada(char *command, tListCom *listcom, tListFic *listafic, int *exit, int storeCommand) {
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
            stat_fun(trozos + 1);

        }else  if (!strcmp(trozos[0], "list")){
            list_fun(trozos + 1);

        } else  if (!strcmp(trozos[0], "delete")){
            delete(trozos + 1);

        } else  if (!strcmp(trozos[0], "deltree")){
            deltree(trozos + 1);

        } else {
            printf("Comando no reconocido\n");
        }
        for (int i = 0; i < numt; i++) {
            free(trozos[i]);
        }
    }
}

int main() {
    char command[MAXLEN];
    int exit = 0;
    int storeCommand = 1;

    tListCom list;
    tListFic listaficheros;
    CreateListCom(&list);
    CreateListFic(&listaficheros);

    AddFic(&listaficheros, 0, O_RDWR, "entrada estandar");
    AddFic(&listaficheros, 1, O_RDWR, "salida estandar");
    AddFic(&listaficheros, 2, O_RDWR, "error estandar");

    while (exit == 0){
        imprimirPrompt();
        leerEntrada(command);
        procesar_entrada(command, &list, &listaficheros, &exit, storeCommand);
    }
    ClearListFic(&listaficheros);
    ClearListCom(&list);
    return 0;
}
