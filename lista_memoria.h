#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifndef UNTITLED_LISTA_MEMORIA_H
#define UNTITLED_LISTA_MEMORIA_H
#define MAXLENC 50

typedef struct tNodeMem{
    char direccion[MAXLENC];
    int size;
    char time_string[20];
    char type_alloc[MAXLENC];
    int key;
    struct tNodeMem *next;
}tNodeMem;

typedef tNodeMem *tPosMem;
typedef tPosMem tListMem;

void CreateListMemBlock(tListMem *list);
void InsertMemBlock(tListMem *list, char *direccion, int size, char *time_string, char * type_alloc, int key)
void ClearListMemBlock(tListMem *list);

#endif
