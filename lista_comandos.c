#include "lista_comandos.h"

void CreateListCom(tListCom *list){
    *list = NULL;
}

void InsertCommand(tListCom *list, char *item) {
    tPosCom newNode = malloc(sizeof(tNodeCom));
    if (newNode == NULL) {
        perror("Error asignando memoria");
        return;
    }

    // Ensure newNode->command is initialized to an empty string
    newNode->command[0] = '\0';

    // Copy the content of item to newNode->command, up to MAXLENC - 1 characters
    strncpy(newNode->command, item, MAXLENC - 1);

    // Ensure newNode->command is null-terminated
    newNode->command[MAXLENC - 1] = '\0';

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