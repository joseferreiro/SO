#include "lista_comandos.h"

void CreateListCom(tListCom *list){
    *list = NULL;
}

void InsertCommand(tListCom *list, char *item[]) {
    tPosCom newNode = malloc(sizeof(tNodeCom));
    if (newNode == NULL) {
        perror("Error asignando memoria");
        return;
    }

    newNode->command[0] = '\0';

    for (int i = 0; (item[i] != NULL) && (i < MAXLENC); i++) {
        strcpy(newNode->command, item[i]);
    }
    newNode->next = NULL;

    if (*list == NULL) {
        *list = newNode;
    } else {
        tPosCom p;
        for (p = *list; p->next != NULL; p = p->next);
        p->next = newNode;
    }
}


void ClearListCom(tListCom *list) {
    if (list == NULL || *list == NULL) {
        return;
    }
    tPosCom current = *list;
    tPosCom next;

    while (current != NULL) {
        next = current->next;
        free(current);
        current = next;
    }
    *list = NULL;
}