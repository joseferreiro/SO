//Autor 1: José Manuel Ferreiro López   login: jose.manuel.ferreiro@udc.es
//Autor 2: Alvaro Mendez Linares    login: alvaro.mendez.linares@udc.es

//FLAGS de tipo de allocation
//revisar tamaño direcciones
//solucionar commandN
//desmapeado no funciona (mmap) listado tampoco (id de tipo de allocation)
//listado de shared no funciona, borrado tampoco (id de tipo de allocation)
//formato lista memoria e impresión (e.g. tipo de asignación) (e.g. mmap y malloc no deben acceder a los mismos bloques)
//revisar dirección en Cmd_malloc
//más cambios de sintaxis !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//revisar comandos read y write
//añador el help de nuevos comandos
//error con el free de dirección en ClearListMem (excepto malloc)
//obtener contenido de la dirección, no del fichero (Write)
//memdump


//rervisar tNodeMem, structs innecesarias
//mucho que revisar en mmap


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

#include <sys/stat.h>
#include <pwd.h>
#include <grp.h>
#include <limits.h>
#include <errno.h>
#include <dirent.h>
#include <ctype.h>
#include <sys/wait.h>
#include <stdbool.h>

#include <sys/mman.h>
#include <sys/shm.h>

#define MAXLEN 200
#define TROZOS 15  //numere de trozos que se pueden almacenar en la array trozos
#define TAMANO 2048

#define LIST_SIZE_OFFSET 0 // Replace with the actual offset value
#define MEM_FLAGS_OFFSET 4 // Replace with the actual offset value
#define MEM_FILE_NAME_OFFSET 8 // Replace with the actual offset value
#define MEM_SIZE_OFFSET 12 // Replace with the actual offset value
#define MEM_FD_OFFSET 16 // Replace with the actual offset value
#define MEM_MMAP_FLAG 0x1 // Replace with the actual flag value
#define MAP_SHARED 0x01  // Replace with the actual value
#define MAP_READ 0x02  // Replace with the actual value
#define MAP_WRITE 0x04  // Replace with the actual value
#define MAP_EXECUTE 0x08  // Replace with the actual value


/*
void RepeatHistoric(undefined8 param_1,uint param_2)

{
    int iVar1;
    undefined local_5018 [16384];
    char local_1018 [4104];
    char *local_10;

    loopHistoric = loopHistoric + 1;
    local_10 = (char *)GetHistoric(LH,param_2);
    if (local_10 == (char *)0x0) {
        printf("No hay elemento %d en el historico\n",(ulong)param_2);
    }
    else {
        strcpy(local_1018,local_10);
        printf("Ejecutando hist (%d): %s\n",(ulong)param_2,local_1018);
        iVar1 = TrocearCadena(local_1018,local_5018);
        if (iVar1 == 0) {
            printf("Nada que ejecutar en el elemento %d del historico\n",(ulong)param_2);
        }
        else if (loopHistoric < 0xb) {
            ProcesarEntrada(local_5018);
        }
        else {
            puts("Demasiada recursion en hist");
        }
    }
    return;
}

 void Cmd_command(char **param_1)

{
  int iVar1;

  if (*param_1 == (char *)0x0) {
    PrintHistoric(LH,0x1000);
  }
  else {
    iVar1 = atoi(*param_1);
    RepeatHistoric(LH,iVar1);
  }
  return;
}
 */

// Function to search for a node by address in the list
int FindNodeByAddress(tListMem *list, ulong address) {
    int index = 0;

    while (true) {
        if (*(int *)(*list + 0x8000) <= index) {
            return -1; // Node not found
        }
        if (address == **(ulong **)(*list + (long)index * 8)) {
            break; // Node found
        }
        index++;
    }

    return index;
}

// Function to read file contents into a buffer
ssize_t LeerFichero (char *f, void *p, size_t cont)
{
    struct stat s;
    ssize_t  n;
    int df,aux;

    if (stat (f,&s)==-1 || (df=open(f,O_RDONLY))==-1)
        return -1;
    if (cont==-1)   /* si pasamos -1 como bytes a leer lo leemos entero*/
        cont=s.st_size;
    if ((n=read(df,p,cont))==-1){
        aux=errno;
        close(df);
        errno=aux;
        return -1;
    }
    close (df);
    return n;
}

// Function to write buffer contents to a file
ssize_t EscribirFichero (char *f, void *p, size_t cont,int overwrite)
{
    ssize_t  n;
    int df,aux, flags=O_CREAT | O_EXCL | O_WRONLY;

    if (overwrite)
        flags=O_CREAT | O_WRONLY | O_TRUNC;

    if ((df=open(f,flags,0777))==-1)
        return -1;

    if ((n=write(df,p,cont))==-1){
        aux=errno;
        close(df);
        errno=aux;
        return -1;
    }
    close (df);
    return n;
}

// Command to read file contents and write them to a memory address

void Cmd_read(char *ar[]) {
    void *p;
    size_t cont = -1;  // -1 indicates reading the entire file
    ssize_t n;

    if (ar[0] == NULL || ar[1] == NULL) {
        printf("Missing parameters\n");
        return;
    }

    // Convert from string to pointer
    p = (void *)strtoul(ar[1], (char **)0, 16);

    if (ar[2] != NULL) {
        // Convert from string to size_t
        cont = (size_t)atoll(ar[2]);
    }

    n = LeerFichero(ar[0], p, cont);

    if (n == -1) {
        perror("Unable to read file");
    } else {
        printf("Read %zd bytes from %s into %p\n", n, ar[0], p);
    }
}

// Command to read a file into a buffer and write it to another file
void Cmd_write(char **commandParameters, tListMem *lista) {
    int overwrite = 0;
    char **parameters = commandParameters;

    if (*parameters == NULL) {
        puts("Missing parameters");
        return;
    }

    // Check for the overwrite flag
    if (strcmp(*parameters, "-o") == 0) {
        overwrite = 1;
        parameters++;
        if (*parameters == NULL) {
            puts("Missing parameters");
            return;
        }
    }

    // Parse file, address, and size
    char *file = parameters[0];
    char *dir_mem_to_read = parameters[1];
    unsigned long userAddress;
    if (sscanf(parameters[1], "%lx", &userAddress) != 1) {
        puts("Invalid address");
        return;
    }

    // Read the size directly from the prompt
    size_t bufferSize;
    if (sscanf(parameters[2], "%zu", &bufferSize) != 1) {
        puts("Invalid buffer size");
        return;
    }

    void *buffer = malloc(bufferSize);
    if (buffer == NULL) {
        perror("Unable to allocate buffer");
        return;
    }

    ssize_t bytesRead = FindNodeByAddress(lista, (ulong) dir_mem_to_read); //---------------------------

    if (bytesRead == -1) {
        perror("Unable to read file");
    } else {
        ssize_t bytesWritten = EscribirFichero(file, buffer, bytesRead, overwrite);

        if (bytesWritten == -1) {
            perror("Unable to write file");
        } else {
            printf("Written %zd bytes to %s from %p\n", bytesWritten, file, (void *)userAddress);
        }
    }

    free(buffer);
}

void doMalloc(size_t tamano, tListMem *lista) {
    void *memoria = malloc(tamano);

    if (memoria == NULL) {
        perror("Imposible obtener memoria con malloc");
    } else {
        printf("Asignados %lu bytes en %p\n", tamano, memoria);
        InsertMemBlock(lista, (ulong)memoria, tamano, 0, 0, NULL, 0);
    }
}

int BuscarNodoTam(tListMem *lista, ulong direccion, uint tipo) {
    int local_c = 0;
    tNodeMem *current = *lista;

    while (current != NULL) {
        if (((current->memoryInfo.allocationType & tipo) != 0) && (direccion == current->direccion)) {
            return local_c;
        }
        local_c++;
        current = current->next;
    }

    return -1;
}

ulong BuscarNodoDireccionMalloc(tListMem *lista, size_t size) {
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

ulong DireccionNodoMalloc(tListMem *lista, size_t size) {
    ulong index = BuscarNodoDireccionMalloc(lista, size);

    if (index == -1) {
        return 0;
    }

    tNodeMem *current = *lista;

    for (ulong i = 0; i < index; ++i) {
        current = current->next;
    }

    return current->direccion;
}

void doMallocDealloc(size_t size, tListMem *lista) {
    ulong direccion = DireccionNodoMalloc(lista, size);//----------------------

    if (direccion == 0) {
        puts("No hay bloque de ese tamaño asignado con malloc");
    } else {
        RemoveNodeByAddress(lista, direccion);
    }
}

void Cmd_memmalloc(char **args, tListMem *lista) {
    if (*args == NULL) {
        PrintList(lista,0);
    } else {
        ulong size = strtoul(*args, NULL, 10);
        if (size == 0) {
            puts("No se asignan bloques de 0 bytes");
        } else {
            doMalloc(size, lista);
        }
    }
}

void Cmd_memdemalloc(char **args, tListMem *lista) {
    if (*args == NULL) {
        PrintList(lista,0);
    } else {
        size_t size = strtoul(*args, NULL, 10);
        if (size == 0) {
            puts("No se asignan bloques de 0 bytes");
        } else {
            doMallocDealloc(size, lista);
        }
    }
}

void Cmd_malloc(char **args, tListMem *lista) {
    if (*args != NULL && strcmp(*args, "-free") == 0) {
        Cmd_memdemalloc(args + 1, lista);
    } else {
        Cmd_memmalloc(args, lista);
    }
}

void Recursiva(int n) {
    char automatico[TAMANO];
    static char estatico[TAMANO];

    printf("parametro:%3d(%p) array %p, arr estatico %p\n", n, &n, automatico, estatico);

    if (n > 0)
        Recursiva(n - 1);
}

void Cmd_Recurse(char **args) {
    if (*args != NULL) {
        int param = atoi(*args);
        Recursiva(param);
    }
}

//-------------------------------------

// Function to insert a memory-mapped node into the list
void InsertMappedNode(tListMem *list, ulong address, ulong size, uint descriptor, char *fileName) {
    // Assuming MEM_TYPE_MMAP_FLAG is the type for memory-mapped nodes
    InsertMemBlock(list, address, size, MEM_TYPE_MMAP_FLAG, descriptor, fileName, MEM_TYPE_MMAP_FLAG);
}

// Function to map a file to memory
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

// Function to print the list of memory-mapped nodes
void PrintMappedList(tListMem *list, ulong tipeAlloc) {
    PrintList(list, MEM_TYPE_MMAP_FLAG);
}

// Command to handle memory mapping
void Cmd_memmap(char **params, tListMem *memList) {
    if (*params == NULL) {
        PrintMappedList(memList, MEM_TYPE_MMAP_FLAG);
    } else {
        char *fileName = *params;

        if (strlen(fileName) < 1) {
            fprintf(stderr, "Error: Invalid file name\n");
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
                fprintf(stderr, "Error: Unable to map file %s\n", fileName);
            } else {
                printf("File %s mapped at %p\n", fileName, address);
            }
        } else {
            fprintf(stderr, "Error: Invalid permissions\n");
        }
    }
}

// Function to get the size of a node by address
ulong GetNodeSizeByAddress(tListMem *list, ulong address) {
    int index = FindNodeByAddress(list, address);
    return (index == -1) ? 0xffffffffffffffff : *(ulong *)(*list + (long)index * 8 + 8);
}

// Function to search for a mapped node by file name
int FindMappedNode(tListMem *list, char *fileName) {
    int index = 0;

    while (true) {
        if (*(int *)(*list + 0x8000) <= index) {
            return -1; // Node not found
        }
        ulong nodeAddress = **(ulong **)(*list + (long)index * 8);
        int allocationType = *(uint *)(nodeAddress + 0x1c);

        if (((allocationType & MEM_TYPE_MMAP_FLAG) != 0) && (*(long *)(nodeAddress + 0x10) != 0)) {
            int result = strcmp(*(char **)(nodeAddress + 0x10), fileName);
            if (result == 0) {
                break; // Node found
            }
        }
        index++;
    }

    return index;
}

// Function to get the address of a mapped node by file name
ulong GetMappedNodeAddress(tListMem *list, char *fileName) {
    int index = FindMappedNode(list, fileName);
    if (index == -1) {
        return 0;
    } else {
        return **(ulong **) (*list + (long) index * 8);
    }
}

// Function to get the descriptor of a mapped node by file name
uint GetMappedNodeDescriptor(tListMem *list, char *fileName) {
    int index = FindMappedNode(list, fileName);
    return (index == -1) ? 0xffffffff : *(uint *)(**(ulong ***)(*list + (long)index * 8) + 0x18);
}

// Function to deallocate a mapped node
void DeallocateMappedNode(char *fileName, tListMem *list) {
    tPosMem prev = NULL;
    tPosMem current = *list;

    while (current != NULL) {
        if (current->type_alloc == MEM_TYPE_MMAP_FLAG && strcmp(current->memoryInfo.otherInfo.mappedFile.fileName, fileName) == 0) {
            size_t size = current->size;
            void *address = (void *)current->direccion;
            int descriptor = current->memoryInfo.otherInfo.mappedFile.fileDescriptor;

            int result = munmap(address, size);

            if (result == -1) {
                int *error = __errno_location();
                char *errorString = strerror(*error);
                printf("Unable to unmap %s at %p: %s\n", fileName, address, errorString);
            } else {
                close(descriptor);
                // Remove from the list
                if (prev == NULL) {
                    *list = current->next;
                } else {
                    prev->next = current->next;
                }
                free(current->memoryInfo.otherInfo.mappedFile.fileName);
                free(current);
            }
            return;
        }
        prev = current;
        current = current->next;
    }
    printf("File %s not mapped\n", fileName);
}

// Command to handle memory deallocation
void Cmd_memdemmap(char **parameters, tListMem *list) {
    if (*parameters == NULL) {
        PrintMappedList(list, 0x119480);
    } else {
        DeallocateMappedNode(*parameters, list);
    }
}

// Main command for memory mapping and deallocation
void Cmd_mmap(char **parameters, tListMem *list) {
    int result;

    if (*parameters != NULL && (result = strcmp(*parameters, "-free"), result == 0)) {
        Cmd_memdemmap(parameters + 1, list);
    } else {
        Cmd_memmap(parameters, list);
    }
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void PrintSharedList(tListMem *lista) {
    PrintList(lista, 2);
}

int FindNodeByKey(tListMem *lista, int key, char type) {
    int index = 0;

    while (true) {
        if (*(int *)(lista + 0x8000) <= index) {
            return -1;
        }
        if ((((int)type & *(uint *)(*(long *)(lista + (long)index * 8) + 0x1c)) != 0) &&
            (key == *(int *)(*(long *)(lista + (long)index * 8) + 0x18))) {
            break;
        }
        index++;
    }

    return index;
}

ulong GetNodeAddressByKey(tListMem *lista, int key) {
    int index = FindNodeByKey(lista, key, '\x02');
    return (index == -1) ? 0 : **(ulong **)(lista + (long)index * 8);
}

void DoSharedDealloc(int key, tListMem *lista) {
    void *sharedAddr = (void *)GetNodeAddressByKey(lista, key);

    if (sharedAddr == (void *)0x0) {
        puts("No hay bloque de esa clave mapeado en el proceso");
    } else {
        shmdt(sharedAddr);
        ulong status = RemoveNodeByAddress(lista, (long)sharedAddr);

        if ((int)status == -1) {
            int *error = __errno_location();
            char *errorString = strerror(*error);
            printf("Imposible quitar de lista: %s\n", errorString);
        }
    }
}

void CmdMemDeShared(char **params, tListMem *lista) {
    ulong key;

    if (*params == (char *)0x0) {
        PrintSharedList(lista);
    } else {
        key = strtoul(*params, (char **)0x0, 10);
        DoSharedDealloc((int)key, lista);
    }
}

void InsertSharedNode(tListMem *lista, ulong address, ulong size, ulong key) { //----------------------------------------------------------------------
    InsertMemBlock(lista, address, size, key, 3, "wwe",5);
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

void CmdCreateShared(char **params, tListMem *lista) {
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

void CmdDeleteKey(char **params) {
    int shmId, result;
    ulong key;

    if (*params != (char *)0x0) {
        key = strtoul(*params, (char **)0x0, 10);
        if ((int)key != 0) {
            shmId = shmget((int)key, 0, 0x1b6);
            if (shmId == -1) {
                perror("shmget: imposible obtener memoria compartida");
                return;
            }
            result = shmctl(shmId, 0, (struct shmid_ds *)0);
            if (result != -1) {
                return;
            }
            perror("shmctl: imposible eliminar memoria compartida\n");
            return;
        }
    }
    puts("      delkey necesita clave_valida");
}

void CmdMemShared(char **params, tListMem  *lista) {
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

void CmdShared(char **params, tListMem *lista) {
    int result;

    if (*params == (char *)0x0) {
        PrintSharedList(lista);
    } else {
        result = strcmp(*params, "-free");
        if (result == 0) {
            CmdMemDeShared(params + 1, lista);
        } else {
            result = strcmp(*params, "-create");
            if (result == 0) {
                CmdCreateShared(params + 1, lista);
            } else {
                result = strcmp(*params, "-delkey");
                if (result == 0) {
                    CmdDeleteKey(params + 1);
                } else {
                    CmdMemShared(params, lista);
                }
            }
        }
    }
}

//-----------------------------------------------------

char *TranslateSpecialChar(char ch) {
    static char buffer[4];  // Buffer for the translated character

    if (ch == '\n') {
        return "  \\n";
    } else if (ch == '\t') {
        return "  \\t";
    } else if (ch == '\r') {
        return "  \\r";
    } else {
        if (-1 < ch && isprint(ch)) {
            snprintf(buffer, sizeof(buffer), "  %c", ch);
            return buffer;
        }
        return "  _"; // Placeholder for non-printable characters
    }
}

void DisplayLine(long address, int length) {
    int index;

    for (index = 0; index < length; index++) {
        char *translatedChar = TranslateSpecialChar(*(char *)(address + index));
        printf("%s", translatedChar);
    }
    putchar('\n');
    for (index = 0; index < length; index++) {
        printf(" %02x", (uint)*(u_char *)(address + index));
    }
    putchar('\n');
}

void DisplayMemory(long address, size_t size) {
    size_t chunkSize;

    for (chunkSize = 0; chunkSize < (size >> 4); chunkSize++) {
        DisplayLine(address, 0x19);
        address += 0x19;
    }
    DisplayLine(address, (int)size % 0x19);
}

void Cmd_memdump(long *commandParams) {
    long address;
    size_t size;

    if (commandParams[0] != 0 && commandParams[1] != 0) {
        address = (long)commandParams[0];
        size = strtoul((char *)commandParams[1], (char **)0x0, 10);
        printf("Dumping %zu bytes from the address %p\n", size, (void *)address);
        DisplayMemory(address, size);
    }
}



//----------------------------------------------------


ulong ConvertByte(char *param)
{
    if (*param == '\'' && param[2] == '\'') {
        return (ulong)(u_char)param[1];
    } else {
        return strtoul(param, (char **)0, 0);
    }
}

void LlenarMemoria (void *p, size_t cont, unsigned char byte)
{
    unsigned char *arr=(unsigned char *) p;
    size_t i;

    for (i=0; i<cont;i++)
        arr[i]=byte;
}

void Cmd_memfill(long *params)
{
    ulong size = 0x80;
    u_char fillByte = 0x41;

    if (params[0] != 0) {
        if (params[1] != 0) {
            size = strtoul((char *)params[1], (char **)0, 10);
            if (params[2] != 0) {
                fillByte = (u_char)ConvertByte((char *)params[2]);
            }
        }

        // Assuming the first parameter is the address in hexadecimal format
        long address = strtol((char *)params[0], (char **)0, 16);
        printf("Filling %zu bytes of memory with byte %lu (%02lx) at address %p\n",
               size, (ulong)fillByte, (ulong)fillByte, (void *)address);

        LlenarMemoria((void *)address, size, fillByte);
    }
}

//----------------------------------------------------



//----------------------------------------------------
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
            perror("cannot execute pmap");
        exit(1);
    }
    waitpid (pid,NULL,0);
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

void procesar_entrada(char *command, tListCom *listcom, tListFic *listafic, tListMem *listamemoria, int *exit, int storeCommand);

void commandN(char *command, tListCom *list, tListFic *listfic, tListMem *listamemoria, int *exit) {
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
        procesar_entrada(commandCopy, list, listfic, listamemoria, exit, storeCommand);
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

    }else if (!strcmp((command), "malloc")){
        printf("4");

    }else if (!strcmp((command), "shared")){
        printf("4");

    }else if (!strcmp((command), "mmap")){
        printf("4");

    }else if (!strcmp((command), "read")){
        printf("4");

    }else if (!strcmp((command), "write")){
        printf("4");

    }else if (!strcmp((command), "memdump")){
        printf("4");

    }else if (!strcmp((command), "memfill")){
        printf("4");

    }else if (!strcmp((command), "mem")){
        printf("4");

    }else if (!strcmp((command),"recurse")){
        printf("4");

    }


    else{
        printf("comando no reconocido.\n");

    }
}

void imprimirPrompt(){
    printf("->");
}

void leerEntrada(char *command){
    fgets(command, MAXLEN, stdin);
}

void procesar_entrada(char *command, tListCom *listcom, tListFic *listafic, tListMem *listamemoria, int *exit, int storeCommand) {
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
            commandN(trozos[1], listcom, listafic, listamemoria, exit);

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
            Cmd_malloc(trozos + 1, listamemoria);

        } else  if (!strcmp(trozos[0], "shared")){
            CmdShared(trozos + 1, listamemoria);

        } else  if (!strcmp(trozos[0], "mmap")){
            Cmd_mmap(trozos + 1, listamemoria);

        } else  if (!strcmp(trozos[0], "read")){
            Cmd_read((trozos + 1));

        } else  if (!strcmp(trozos[0], "write")){
            Cmd_write(trozos + 1, listamemoria);

        } else  if (!strcmp(trozos[0], "memdump")){
            Cmd_memdump((long *) (trozos + 1));

        } else  if (!strcmp(trozos[0], "memfill")){
            Cmd_memfill((long *) (trozos + 1));

        } else  if (!strcmp(trozos[0], "recurse")){
            Cmd_Recurse(trozos + 1);

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
    tListMem listamemoria;
    CreateListCom(&list);
    CreateListFic(&listaficheros);
    CreateListMemBlock(&listamemoria);

    AddFic(&listaficheros, 0, O_RDWR, "entrada estandar");
    AddFic(&listaficheros, 1, O_RDWR, "salida estandar");
    AddFic(&listaficheros, 2, O_RDWR, "error estandar");

    while (exit == 0){
        imprimirPrompt();
        leerEntrada(command);
        procesar_entrada(command, &list, &listaficheros, &listamemoria, &exit, storeCommand);
    }
    ClearListFic(&listaficheros);
    ClearListCom(&list);
    ClearListMemBlock(&listamemoria);
    return 0;
}
