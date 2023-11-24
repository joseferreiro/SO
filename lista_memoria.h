#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifndef UNTITLED_LISTA_MEMORIA_H
#define UNTITLED_LISTA_MEMORIA_H
#define MAXLENC 50
#define MEM_TYPE_MMAP_FLAG    1
#define MEM_TYPE_MALLOC_FLAG  0
#define MEM_TYPE_SHARED_FLAG  2


typedef struct tNodeMem {
    ulong direccion;
    size_t size;
    time_t time_string;
    int type_alloc;

    struct {
        int allocationType; //
        union {
            ulong sharedMemoryKey;    // Usado para shared
            struct {
                char* fileName;      // usado para mmap
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
void PrintList(tListMem *list, uint type_alloc);
int RemoveNodeByAddress(tListMem* list, ulong direccion);
void ClearListMemBlock(tListMem *list);
tPosMem FindNodeByAddressMem(tListMem *lista, ulong address);

#endif
