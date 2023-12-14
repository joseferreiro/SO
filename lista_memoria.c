#include <time.h>
#include <unistd.h>
#include <sys/shm.h>
#include <sys/mman.h>
#include "lista_memoria.h"

void CreateListMemBlock(tListMem *list) {
    *list = NULL;
}

void InsertMemBlock(tListMem *list, ulong direccion, size_t size, int type_alloc, int Key, const char *fileName, int fileDescriptor) {
    tPosMem newNode = malloc(sizeof(tNodeMem));

    if (newNode == NULL) {
        perror("Error asignando memoria");
        return;
    }

    newNode->direccion = direccion;
    newNode->size = size;
    newNode->time_string = time(NULL);
    newNode->type_alloc = type_alloc;

    if (type_alloc == MEM_TYPE_SHARED_FLAG) {
        newNode->memoryInfo.allocationType = type_alloc;
        newNode->memoryInfo.otherInfo.sharedMemoryKey = Key;
    } else if (type_alloc == MEM_TYPE_MMAP_FLAG) {
        newNode->memoryInfo.allocationType = type_alloc;
        newNode->memoryInfo.otherInfo.mappedFile.fileName = strdup(fileName);
        newNode->memoryInfo.otherInfo.mappedFile.fileDescriptor = fileDescriptor;
    } else if (type_alloc == MEM_TYPE_MALLOC_FLAG) {
        newNode->memoryInfo.allocationType = type_alloc;
    }

    newNode->next = NULL;

    if (*list == NULL) {
        *list = newNode;
    } else {
        tPosMem p;
        for (p = *list; p->next != NULL; p = p->next)
            ;
        p->next = newNode;
    }
}

void printMemoryBlockTypeHeader(uint type_alloc) {
    char *blockTypeString = "";

    if (type_alloc == MEM_TYPE_MMAP_FLAG) {
        blockTypeString = " mmap";
    } else if (type_alloc == MEM_TYPE_MALLOC_FLAG) {
        blockTypeString = " malloc";
    } else if (type_alloc == MEM_TYPE_SHARED_FLAG) {
        blockTypeString = " shared";
    }

    pid_t processId = getpid();
    printf("******Lista de bloques%s asignados para el proceso %lu\n", blockTypeString, (long)processId);
}

void printMemoryBlock(tNodeMem *block) {
    char *formattedDate = ctime(&block->time_string);
    formattedDate[strlen(formattedDate) - 1] = '\0';

    printf("0x%lx  %15lu %s", block->direccion, block->size, formattedDate);

    int blockType = block->type_alloc;
    if (blockType == MEM_TYPE_MMAP_FLAG) {
        printf(" %s  (descriptor %d)\n", block->memoryInfo.otherInfo.mappedFile.fileName, block->memoryInfo.otherInfo.mappedFile.fileDescriptor);
    } else if (blockType == MEM_TYPE_MALLOC_FLAG) {
        puts(" malloc");
    } else if (blockType == MEM_TYPE_SHARED_FLAG) {
        printf(" shared (key %lu)\n", block->memoryInfo.otherInfo.sharedMemoryKey);
    } else {
        puts("  desconocido");
    }
}

void PrintList(tListMem *memoryList, uint type_alloc) {
    tPosMem current = *memoryList;

    printMemoryBlockTypeHeader(type_alloc);

    while (current != NULL) {
        if (current->type_alloc == type_alloc) {
            printMemoryBlock(current);
        }
        current = current->next;
    }
}

int RemoveNodeByAddress(tListMem *list, ulong address) {
    if (list == NULL || *list == NULL || address == 0) {
        return -1;
    }

    tPosMem current = *list;
    tPosMem prev = NULL;

    while (current != NULL && current->direccion != address) {
        prev = current;
        current = current->next;
    }

    if (current == NULL) {
        return -1;
    }

    if (current->memoryInfo.allocationType == MEM_TYPE_MMAP_FLAG) {
        free(current->memoryInfo.otherInfo.mappedFile.fileName);
    } else if (current->memoryInfo.allocationType == MEM_TYPE_MALLOC_FLAG) {
        free((void *)current->direccion);
    } else if (current->memoryInfo.allocationType == MEM_TYPE_SHARED_FLAG) {
        int shmId = shmget((key_t)current->memoryInfo.otherInfo.sharedMemoryKey, 0, 0);
        if (shmId != -1) {
            shmctl(shmId, IPC_RMID, NULL);
        }
    }

    if (prev == NULL) {
        *list = current->next;
    } else {
        prev->next = current->next;
    }

    current->next = NULL;
    free(current);

    return 0;
}

void ClearListMemBlock(tListMem *list) {
    if (list == NULL || *list == NULL) {
        return;
    }

    tPosMem current = *list;
    tPosMem next;

    while (current != NULL) {
        if (current->memoryInfo.allocationType == MEM_TYPE_MMAP_FLAG) {
            if (current->memoryInfo.otherInfo.mappedFile.fileName != NULL) {
                free(current->memoryInfo.otherInfo.mappedFile.fileName);
            }
            if (current->direccion != 0) {
                munmap((void *)current->direccion, current->size);
            }
        } else if (current->memoryInfo.allocationType == MEM_TYPE_MALLOC_FLAG) {
            free((void *)current->direccion);
        } else if (current->memoryInfo.allocationType == MEM_TYPE_SHARED_FLAG) {
            int shmId = shmget((key_t) current->memoryInfo.otherInfo.sharedMemoryKey, 0, 0);
            if (shmId != -1) {
                shmctl(shmId, IPC_RMID, NULL);
            }
        }

        next = current->next;
        free(current);
        current = next;
    }
    *list = NULL;
}

tPosMem FindNodeByAddressMem(tListMem *lista, ulong address) {
    tPosMem current = *lista;

    while (current != NULL) {
        if (current->direccion == address) {
            return current;
        }
        current = current->next;
    }

    return NULL;
}

int FindNodeByKey(tListMem *lista, int key, int type) {
    int index = 0;

    tPosMem current = *lista;

    while (current != NULL) {
        if (type == '\x02' && current->type_alloc == 2 && key == (int)current->memoryInfo.otherInfo.sharedMemoryKey) {
            return index;
        }
        current = current->next;
        index++;
    }

    return -1;
}