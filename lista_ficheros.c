#include "lista_ficheros.h"

void CreateListFic(tListFic *list){
    *list = NULL;
}

void AñadirAFicherosAbiertos(tListFic *list, int descriptor, int mode, char *nombre) {
    tNodeFic *newNode = (tNodeFic *)malloc(sizeof(tNodeFic));
    if (newNode == NULL) {
        perror("Error asignando memoria");
        return;
    }

    newNode->descriptor = descriptor;
    newNode->mode = mode;
    strncpy(newNode->nombre, nombre, MAXLENF);
    newNode->next = NULL;

    if (*list == NULL) {
        *list = newNode;
    } else {
        tNodeFic *current = *list;
        while (current->next != NULL) {
            current = current->next;
        }
        current->next = newNode;
    }
}

void RemoveFic(tListFic *list, int df) {
    tPosFic current = *list;
    tPosFic prev = NULL;

    while (current != NULL) {
        if (current->descriptor == df) {
            if (prev == NULL) {
                *list = current->next;
            } else {
                prev->next = current->next;
            }
            free(current);
            return;
        }
        prev = current;
        current = current->next;
    }

    printf("Fichero %d no encontrado en la lista.\n", df);
}

void ClearListFic(tListFic *list) {
    if (list == NULL || *list == NULL) {
        return;
    }

    tPosFic current = *list;
    tPosFic next;

    while (current != NULL) {
        next = current->next;
        free(current);
        current = next;
    }
    *list = NULL;
}