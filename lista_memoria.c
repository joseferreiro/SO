#include <time.h>
#include <unistd.h>
#include "lista_memoria.h"

void CreateListMemBlock(tListMem *list) {
    *list = NULL;
}

void InsertMemBlock(tListMem *list, ulong direccion, size_t size, int type_alloc,
                    int Key, const char *fileName, int fileDescriptor) {
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

void printMemoryBlockTypeHeader(uint blockType) {
    char *blockTypeString = "";

    if (blockType == MEM_TYPE_MMAP_FLAG) {
        blockTypeString = " mmap";
    } else if (blockType == MEM_TYPE_MALLOC_FLAG) {
        blockTypeString = " malloc";
    } else if (blockType == MEM_TYPE_SHARED_FLAG) {
        blockTypeString = " shared";
    }

    pid_t processId = getpid();
    printf("******List of allocated blocks%s for the process %lu\n", blockTypeString, (long)processId);
}

void printMemoryBlock(tNodeMem *block) {
    char *formattedDate = ctime(&block->time_string);
    formattedDate[strlen(formattedDate) - 1] = '\0'; // Remove the newline character

    printf("0x%lx  %15lu %s", block->direccion, block->size, formattedDate);

    int blockType = block->type_alloc;
    if (blockType == MEM_TYPE_MMAP_FLAG) {
        printf(" %s  (descriptor %d)\n", block->memoryInfo.otherInfo.mappedFile.fileName, block->memoryInfo.otherInfo.mappedFile.fileDescriptor);
    } else if (blockType == MEM_TYPE_MALLOC_FLAG) {
        puts(" malloc");
    } else if (blockType == MEM_TYPE_SHARED_FLAG) {
        printf(" shared (key %lu)\n", block->memoryInfo.otherInfo.sharedMemoryKey);
    } else {
        puts("  unknown");
    }
}

void PrintList(tListMem *memoryList, uint blockType) {
    tPosMem current = *memoryList;

    printMemoryBlockTypeHeader(blockType);

    while (current != NULL) {
        if (current->type_alloc == blockType) {
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
        // Node with the specified address not found
        return -1;
    }

    free(current->memoryInfo.otherInfo.mappedFile.fileName);
    if (prev == NULL) {
        // Node to be removed is the first node in the list
        *list = current->next;
    } else {
        prev->next = current->next;
    }
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
        free(current->memoryInfo.otherInfo.mappedFile.fileName);
        next = current->next;
        free(current);
        current = next;
    }
    *list = NULL;
}