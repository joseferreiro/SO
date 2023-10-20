#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define MAXLENF 50

#ifndef UNTITLED_LISTA_FICHEROS_H
#define UNTITLED_LISTA_FICHEROS_H

typedef struct tNodeFic{
    char nombre[MAXLENF];
    int mode;
    int descriptor;
    struct tNodeFic *next;
}tNodeFic;

typedef tNodeFic *tPosFic;
typedef tPosFic tListFic;

void CreateListFic(tListFic *list);
void AñadirAFicherosAbiertos(tListFic *list, int descriptor, int mode, char *nombre);
void RemoveFic(tListFic *list, int df);
void ClearListFic(tListFic *list);

#endif
