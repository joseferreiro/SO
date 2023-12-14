#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include "lista_comandos.h"
#include "lista_ficheros.h"
#include "lista_memoria.h"
#include "comandos_P2.h"


#include <sys/stat.h>
#include <errno.h>
#include <sys/wait.h>

#include <sys/mman.h>
#include <sys/shm.h>

int gd1 = 32, gd2 = 423, gd3 = 4376;  //variables para mem
int gnd1, gnd2, gnd3;
static int gsd1 = 342, gsd2 = 423, gsd3=324;
static int gsnd1, gsnd2, gsnd3;

ssize_t LeerFichero(char *f, void *p, size_t cont) {
    struct stat s;
    ssize_t n;
    int df, aux;

    if (f == NULL || p == NULL) {
        fprintf(stderr, "Invalid parameters\n");
        return -1;
    }

    if (stat(f, &s) == -1 || (df = open(f, O_RDONLY)) == -1) {
        perror("Unable to open file");
        return -1;
    }

    if (cont == -1)   /* if we pass -1 as bytes to read, read the entire file */
        cont = s.st_size;

    if ((n = read(df, p, cont)) == -1) {
        aux = errno;
        close(df);
        errno = aux;
        perror("Unable to read file");
        return -1;
    }

    close(df);
    return n;
}

ssize_t EscribirFichero(char *f, void *p, size_t cont, int overwrite) {
    ssize_t n;
    int df, aux, flags = O_CREAT | O_EXCL | O_WRONLY;

    if (overwrite)
        flags = O_CREAT | O_WRONLY | O_TRUNC;

    if ((df = open(f, flags, 0777)) == -1)
        return -1;

    if ((n = write(df, p, cont)) == -1) {
        aux = errno;
        close(df);
        errno = aux;
        return -1;
    }
    close(df);
    return n;
}

void read_fun(char *ar[]) {
    void *p;
    size_t cont = -1;  // -1 indica leer todo el fichero
    ssize_t n;

    if (ar[0] == NULL || ar[1] == NULL) {
        fprintf(stderr, "Faltan parametros\n");
        return;
    }

    p = (void *)strtoul(ar[1], (char **)0, 16);

    if (ar[2] != NULL) {
        cont = (size_t)atoll(ar[2]);
    }

    n = LeerFichero(ar[0], p, cont);

    if (n != -1) {
        printf("Leidos %zd bytes de %s a %p\n", n, ar[0], p);
    }
}

void write_fun(char **params, tListMem *lista) {
    int overwrite = 0;
    char **parameters = params;

    if (*parameters == NULL) {
        puts("Faltan parametros");
        return;
    }

    if (strcmp(*parameters, "-o") == 0) {
        overwrite = 1;
        parameters++;
        if (*parameters == NULL) {
            puts("Faltan parametros");
            return;
        }
    }

    char *file = parameters[0];
    unsigned long userAddress;
    if (sscanf(parameters[1], "%lx", &userAddress) != 1) {
        puts("Dirección no válida");
        return;
    }

    size_t bufferSize;
    if (sscanf(parameters[2], "%zu", &bufferSize) != 1) {
        puts("Tamaño del buffer no válido");
        return;
    }

    tPosMem node = FindNodeByAddressMem(lista, (ulong)userAddress);
    if (node == NULL) {
        perror("No se encontró la dirección de memoria");
        return;
    }

    if (bufferSize > node->size) {
        puts("Error con el tamaño de buffer");
        return;
    }

    void *buffer = (void *)userAddress;

    ssize_t bytesWritten = EscribirFichero(file, buffer, bufferSize, overwrite);

    if (bytesWritten == -1) {
        perror("No se pudo escribir el fichero");
    } else {
        printf("Escritos %zd bytes a %s desde %p\n", bytesWritten, file, (void *)userAddress);
    }
}

void MallocAux(size_t tamano, tListMem *lista) {
    void *memoria = malloc(tamano);

    if (memoria == NULL) {
        perror("Imposible obtener memoria con malloc");
    } else {
        printf("Asignados %lu bytes en %p\n", tamano, memoria);
        InsertMemBlock(lista, (ulong)memoria, tamano, MEM_TYPE_MALLOC_FLAG, 0, NULL, 0);
    }
}

ulong GetNodeAddressMalloc(tListMem *lista, size_t size) {
    int local_c = 0;
    tNodeMem *current = *lista;

    while (current != NULL) {
        if (size == current->size) {
            return local_c;
        }
        local_c++;
        current = current->next;
    }

    return -1;
}

ulong NodeAddressMalloc(tListMem *lista, size_t size) {
    ulong index = GetNodeAddressMalloc(lista, size);

    if (index == -1) {
        return 0;
    }

    tNodeMem *current = *lista;

    for (ulong i = 0; i < index; ++i) {
        current = current->next;
    }

    return current->direccion;
}

void Dealloc(size_t size, tListMem *lista) {
    ulong direccion = NodeAddressMalloc(lista, size);

    if (direccion == 0) {
        puts("No hay bloques de ese tamaño asignado con malloc");
    } else {
        RemoveNodeByAddress(lista, direccion);
    }
}

void Malloc(char **args, tListMem *lista) {
    if (*args == NULL) {
        PrintList(lista,0);
    } else {
        ulong size = strtoul(*args, NULL, 10);
        if (size == 0) {
            puts("No se asignan bloques de 0 bytes");
        } else {
            MallocAux(size, lista);
        }
    }
}

void Demalloc(char **args, tListMem *lista) {
    if (*args == NULL) {
        PrintList(lista,0);
    } else {
        size_t size = strtoul(*args, NULL, 10);
        if (size == 0) {
            puts("No se asignan bloques de 0 bytes");
        } else {
            Dealloc(size, lista);
        }
    }
}

void malloc_fun(char **args, tListMem *lista) {
    if (*args != NULL && strcmp(*args, "-free") == 0) {
        Demalloc(args + 1, lista);
    } else {
        Malloc(args, lista);
    }
}

void Recursiva(int n) {
    char automatico[TAMANO];
    static char estatico[TAMANO];

    printf("parametro:%3d(%p) array %p, arr estatico %p\n", n, &n, automatico, estatico);

    if (n > 0)
        Recursiva(n - 1);
}

void Recurse(char **args) {
    if (*args != NULL) {
        int param = atoi(*args);
        Recursiva(param);
    }
}



void InsertMappedNode(tListMem *list, ulong address, ulong size, uint descriptor, char *fileName) {
    InsertMemBlock(list, address, size, MEM_TYPE_MMAP_FLAG, descriptor, fileName, 0);
}

void *MapFileToMemory(char *fileName, uint flags, tListMem *list) {
    int openFlags = (flags & MAP_SHARED) ? O_RDWR : O_RDONLY;
    int fd = open(fileName, openFlags);

    if (fd == -1) {
        perror("Unable to open file");
        return NULL;
    }

    struct stat fileStat;
    if (fstat(fd, &fileStat) == -1) {
        perror("Unable to get file status");
        close(fd);
        return NULL;
    }

    void *address = mmap(NULL, fileStat.st_size, flags, MAP_PRIVATE, fd, 0);

    if (address == MAP_FAILED) {
        perror("Unable to map file");
        close(fd);
        return NULL;
    }

    InsertMappedNode(list, (ulong)address, fileStat.st_size, (ulong)fd, fileName);
    return address;
}

void PrintMappedList(tListMem *list) {
    PrintList(list, MEM_TYPE_MMAP_FLAG);
}

void mmap_Aux(char **params, tListMem *memList) {
    if (*params == NULL) {
        PrintMappedList(memList);
    } else {
        char *fileName = *params;

        if (strlen(fileName) < 1) {
            fprintf(stderr, "Error: Nombre no válido\n");
            return;
        }

        size_t fileNameLength;
        char *permissions;
        ulong flags;

        permissions = params[1];

        if (permissions != NULL && (fileNameLength = strlen(permissions)) < 4) {
            flags = 0;
            if (strchr(permissions, 'r') != NULL) {
                flags |= MAP_READ;
            }
            if (strchr(permissions, 'w') != NULL) {
                flags |= MAP_WRITE;
            }
            if (strchr(permissions, 'x') != NULL) {
                flags |= MAP_EXECUTE;
            }

            void *address = MapFileToMemory(fileName, flags, memList);

            if (address == NULL) {
                fprintf(stderr, "Error: No se pudo mapear el archivo %s\n", fileName);
            } else {
                printf("Archivo %s mapeado a %p\n", fileName, address);
            }
        } else {
            fprintf(stderr, "Error: Permisos no válidos\n");
        }
    }
}

void DeallocateMappedNode(char *fileName, tListMem *list) {
    tPosMem current = *list;

    while (current != NULL) {
        if (current->type_alloc == MEM_TYPE_MMAP_FLAG && strcmp(current->memoryInfo.otherInfo.mappedFile.fileName, fileName) == 0) {
            size_t size = current->size;
            void *address = (void *)current->direccion;

            int result = munmap(address, size);

            if (result == -1) {
                int *error = __errno_location();
                char *errorString = strerror(*error);
                printf("No se pudo desmapear %s en %p: %s\n", fileName, address, errorString);
            } else {
                RemoveNodeByAddress(list, (ulong) address);
            }
            return;
        }
    }
    printf("Archivo %s no mapeado\n", fileName);
}

void demmap(char **parameters, tListMem *list) {
    if (*parameters == NULL) {
        PrintMappedList(list);
    } else {
        DeallocateMappedNode(*parameters, list);
    }
}

void mmap_fun(char **parameters, tListMem *list) {
    int result;

    if (*parameters != NULL && (result = strcmp(*parameters, "-free"), result == 0)) {
        demmap(parameters + 1, list);
    } else {
        mmap_Aux(parameters, list);
    }
}




void PrintSharedList(tListMem *lista) {
    PrintList(lista, 2);
}

void DeallocShared(int key, tListMem *lista) {
    int index = FindNodeByKey(lista, key, '\x02');

    if (index == -1) {
        puts("No hay bloque de esa clave mapeado en el proceso");
    } else {
        void *sharedAddr = (void *)(*lista)->direccion;
        ulong status = RemoveNodeByAddress(lista, (long)sharedAddr);

        if ((int)status == -1) {
            int *error = __errno_location();
            char *errorString = strerror(*error);
            printf("Imposible quitar de lista: %s\n", errorString);
        } else {
            shmdt(sharedAddr);
            printf("Bloque liberado.\n");
        }
    }
}

void Deshare(char **params, tListMem *lista) {
    ulong key;

    if (*params == (char *)0x0) {
        PrintSharedList(lista);
    } else {
        key = strtoul(*params, (char **)0x0, 10);
        DeallocShared((int) key, lista);
    }
}

void InsertSharedNode(tListMem *lista, ulong address, ulong size, ulong key) {
    InsertMemBlock(lista, address, size, MEM_TYPE_SHARED_FLAG, key, NULL, 0);
}

void *GetShmMemory(int key, size_t size, tListMem *lista) {
    int *errorPtr;
    struct shmid_ds localStat;
    int shmFlags = 0x1ff;

    if (size != 0) {
        shmFlags = 0x7ff;
    }

    if (key == 0) {
        errorPtr = __errno_location();
        *errorPtr = 0x16;
        return (void *)0x0;
    }

    int shmId = shmget(key, size, shmFlags);
    if (shmId == -1) {
        return (void *)0x0;
    }

    void *shmAddr = shmat(shmId, (void *)0x0, 0);

    if (shmAddr == (void *)0xffffffffffffffff) {
        errorPtr = __errno_location();
        int localErr = *errorPtr;

        if (size != 0) {
            shmctl(shmId, 0, (struct shmid_ds *)0);
        }

        errorPtr = __errno_location();
        *errorPtr = localErr;
        return (void *)0x0;
    } else {
        shmctl(shmId, 2, &localStat);
        InsertSharedNode(lista, (ulong) shmAddr, localStat.shm_segsz, key);
    }
    return shmAddr;
}

void CreateShared(char **params, tListMem *lista) {
    ulong key, size;
    void *memoryAddr;
    int *errorPtr;
    char *errorString;

    if ((*params == (char *)0x0) || (params[1] == (char *)0x0)) {
        PrintSharedList(lista);
    } else {
        key = strtoul(*params, (char **)0x0, 10);
        size = strtoul(params[1], (char **)0x0, 10);

        if (size == 0) {
            puts("No se asignan bloques de 0 bytes");
        } else {
            memoryAddr = GetShmMemory((int)key, size, lista);

            if (memoryAddr == (void *)0x0) {
                errorPtr = __errno_location();
                errorString = strerror(*errorPtr);
                printf("Imposible asignar memoria compartida clave %lu:%s\n", (long)(int)key, errorString);
            } else {
                printf("Asignados %lu bytes en %p\n", size, memoryAddr);
            }
        }
    }
}

void DeleteKey(char **params, tListMem *lista) {
    int shmId, result;
    int key;

    if (*params != NULL) {
        key = strtoul(*params, NULL, 10);
        if ((int)key != 0) {
            shmId = shmget((int)key, 0, 0x1b6);
            if (shmId == -1) {
                perror("shmget: imposible obtener memoria compartida");
                return;
            }
            result = shmctl(shmId, 0, (struct shmid_ds *)0);
            if (result != -1) {
                ulong address = FindNodeByKey(lista, key, MEM_TYPE_SHARED_FLAG);
                RemoveNodeByAddress(lista, address);
                return;
            }
            perror("shmctl: imposible eliminar memoria compartida\n");
            return;
        }
    }
    puts("delkey necesita una clave valida");
}

void MemShared(char **params, tListMem  *lista) {
    int shmKey;
    ulong key;
    void *memoryAddr;
    int *errorPtr;
    char *errorString;

    if (*params == (char *)0x0) {
        PrintSharedList(lista);
    } else {
        key = strtoul(*params, (char **)0x0, 10);
        shmKey = (int)key;
        memoryAddr = GetShmMemory(shmKey, 0, lista);

        if (memoryAddr == (void *)0x0) {
            errorPtr = __errno_location();
            errorString = strerror(*errorPtr);
            printf("Imposible asignar memoria compartida clave %lu:%s\n", (long)shmKey, errorString);
        } else {
            printf("Memoria compartida de clave %lu en %p\n", (long)shmKey, memoryAddr);
        }
    }
}

void Shared(char **params, tListMem *lista) {
    int result;

    if (*params == (char *)0x0) {
        PrintSharedList(lista);
    } else {
        result = strcmp(*params, "-free");
        if (result == 0) {
            Deshare(params + 1, lista);
        } else {
            result = strcmp(*params, "-create");
            if (result == 0) {
                CreateShared(params + 1, lista);
            } else {
                result = strcmp(*params, "-delkey");
                if (result == 0) {
                    DeleteKey(params + 1, lista);
                } else {
                    MemShared(params, lista);
                }
            }
        }
    }
}



void Show_Mem(void *address, size_t size) {
    uint8_t *ptr = (uint8_t *)address;

    for (size_t i = 0; i < size; ++i) {
        printf("%02x ", ptr[i]);
    }

    putchar('\n');
}

unsigned long long cadtop(char *str) {
    unsigned long long result = 0;
    sscanf(str, "%llx", &result);
    return result;
}

void memdump(char **params) {
    unsigned long long count = 0x19; // Default count
    if (params[1] != NULL) {
        count = strtoull(params[1], (char **)0x0, 10);
    }

    unsigned long long address = cadtop(params[0]);
    printf("Dumping %llu bytes from address %p\n", count, (void *)address);
    Show_Mem((void *) address, count);
}



ulong ConvertByte(const char *param) {
    if (param[0] == '\'' && param[2] == '\'' && param[3] == '\0') {
        return (ulong)(u_char)param[1];
    } else {
        return strtoul(param, NULL, 0);
    }
}

void LlenarMemoria (void *p, size_t cont, unsigned char byte) {
    unsigned char *arr=(unsigned char *) p;
    size_t i;

    for (i=0; i<cont;i++)
        arr[i]=byte;
}

void memfill(long *params) {
    if (params[0] == 0) {
        printf("Faltan parametros\n");
        return;
    }

    long address = strtol((char *)params[0], NULL, 16);
    if (address == 0 && errno == EINVAL) {
        perror("Dirección no válida");
        return;
    }

    ulong size = 0x80;
    if (params[1] != 0) {
        size = strtoul((char *)params[1], NULL, 10);
        if (size == 0 && errno == EINVAL) {
            perror("Tamaño no válido");
            return;
        }
    }

    u_char fillByte = 0x41;
    if (params[2] != 0) {
        fillByte = (u_char)ConvertByte((char *)params[2]);
        if (fillByte == 0 && errno == EINVAL) {
            perror("Byte no válido");
            return;
        }
    }

    printf("Llenando %zu bytes de memoria con el byte %lu (%02lx) en la dirección %p\n",
           size, (ulong)fillByte, (ulong)fillByte, (void *)address);

    LlenarMemoria((void *)address, size, fillByte);
}



void PrintListMem(tListMem *lista) {
    PrintList(lista, 0);
    PrintList(lista, 1);
    PrintList(lista, 2);
}

void print_funcs(void);

void print_vars(void) {
    u_char local1[4];
    u_char local2[4];
    u_char local3[4];

    printf("Variables locales: %35p, %18p, %18p\n", local1, local2, local3);
    printf("Variables globales: %35p, %18p, %18p\n", &gd1, &gd2, &gd3);
    printf("Variables globales no inicializadas: %18p, %18p, %18p\n", &gnd1, &gnd2, &gnd3);
    printf("Variables estáticas: %34p, %18p, %18p\n", &gsd1, &gsd2, &gsd3);
    printf("Variables estáticas no inicializadas: %17p, %18p, %18p\n", &gsnd1, &gsnd2, &gsnd3);
}

void print_pmap(void) {
    u_char local_38[44];
    int myVariable;
    char *pmapArgs[] = {"print_pmap", (char *)local_38, NULL};
    char *procstatArgs[] = {"procstat", (char *)&myVariable, (char *)local_38, NULL};
    char *procmapArgs[] = {"procmap", (char *)local_38, NULL};
    char *vmmapArgs[] = {"vmmap", "-interleave", (char *)local_38, NULL};

    pid_t child_pid = fork();

    if (child_pid == -1) {
        perror("No se pudo crear el proceso");
    } else if (child_pid == 0) {
        if (execvp(pmapArgs[0], pmapArgs) == -1 &&
            execvp(procstatArgs[0], procstatArgs) == -1 &&
            execvp(procmapArgs[0], procmapArgs) == -1 &&
            execvp(vmmapArgs[0], vmmapArgs) == -1) {
            perror("No se pudieron ejecutar los comandos");
            exit(1);
        }
    } else {
        waitpid(child_pid, NULL, 0);
    }
}

void mem(char **param_1, tListMem *lista) {
    if (*param_1 != NULL) {
        if (strcmp(*param_1, "-blocks") == 0) {
            PrintListMem(lista);
        } else if (strcmp(*param_1, "-funcs") == 0) {
            print_funcs();
        } else if (strcmp(*param_1, "-vars") == 0) {
            print_vars();
        } else if (strcmp(*param_1, "-all") == 0) {
            print_vars();
            print_funcs();
            PrintListMem(lista);
        } else if (strcmp(*param_1, "-pmap") == 0) {
            print_pmap();
        } else {
            printf("Parametros no válidos\n");
        }
    } else {
        print_vars();
        print_funcs();
        PrintListMem(lista);
    }
}