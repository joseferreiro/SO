#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifndef UNTITLED_LISTA_MEMORIA_H
#define UNTITLED_LISTA_MEMORIA_H
#define MAXLENC 50
#define MEM_TYPE_MMAP_FLAG    4
#define MEM_TYPE_MALLOC_FLAG  0
#define MEM_TYPE_SHARED_FLAG  2


typedef struct tNodeMem {
    ulong direccion;
    size_t size;
    time_t time_string;
    int type_alloc;

    // Integration of MemoryBlock structure
    struct {
        int allocationType; // 0: malloc memory, 1: shared memory, 2: mapped file
        union {
            ulong sharedMemoryKey;    // Used when allocationType is 1
            struct {
                char* fileName;      // Used when allocationType is 2
                int fileDescriptor;
            } mappedFile;
        } otherInfo;
    } memoryInfo;

    struct tNodeMem* next;
} tNodeMem;

typedef tNodeMem *tPosMem;
typedef tPosMem tListMem;

void CreateListMemBlock(tListMem *list);
void InsertMemBlock(tListMem *list, ulong direccion, size_t size, int type_alloc, int Key, const char* fileName, int fileDescriptor);
void PrintList(tListMem *list, uint blockType);
int RemoveNodeByAddress(tListMem* list, ulong address);
void ClearListMemBlock(tListMem *list);

#endif
