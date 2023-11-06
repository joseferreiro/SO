#include "lista_memoria.h"

void CreateListMemBlock(tListMem *list){
    *list = NULL;
}

void InsertMemBlock(tListMem *list, char *direccion, int size, char *time_string, char *type_alloc, int key) {
    tPosMem newNode = malloc(sizeof(tNodeMem));
    if (newNode == NULL) {
        perror("Error asignando memoria");
        return;
    }

    newNode->direccion[0] = '\0';
    newNode->time_string[0] = '\0';
    newNode->type_alloc[0] = '\0';

    strcpy(newNode->direccion, direccion);
    strcpy(newNode->time_string, time_string);
    strcpy(newNode->type_alloc, type_alloc);
    newNode->size = size;
    newNode->key = key;

    newNode->next = NULL;

    if (*list == NULL) {
        *list = newNode;
    } else {
        tPosMem p;
        for (p = *list; p->next != NULL; p = p->next);
        p->next = newNode;
    }
}

void ClearListMemBlock(tListMem *list) {
    if (list == NULL || *list == NULL) {
        return;
    }
    tPosMem current = *list;
    tPosMem next;

    while (current != NULL) {
        next = current->next;
        free(current);
        current = next;
    }
    *list = NULL;
}