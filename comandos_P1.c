#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <fcntl.h>
#include "lista_comandos.h"
#include "lista_ficheros.h"
#include "lista_memoria.h"
#include "comandos_P0.h"
#include "comandos_P1.h"

#include <sys/stat.h>
#include <pwd.h>
#include <grp.h>
#include <limits.h>
#include <errno.h>
#include <dirent.h>

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

char *ConvierteModo2 (mode_t m) {
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