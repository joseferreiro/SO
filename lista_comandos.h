#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define MAXLENC 50

#ifndef LISTA_COMANDOS
#define LISTA_COMANDOS

typedef struct tNodeCom{
    char command[MAXLENC];
    struct tNodeCom *next;
}tNodeCom;

typedef tNodeCom *tPosCom;
typedef tPosCom tListCom;

void CreateListCom(tListCom *list);
void InsertCommand(tListCom *list, char *command);
void ClearListCom(tListCom *list);

#endif
