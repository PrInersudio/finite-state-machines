#include "EquivalenceClass.h"
#include <stdlib.h>
#include <stdio.h>

void printEquivalenceClass(List *class, PrintValue printState) {
    printf("{ ");
    printList(class, printState);
    printf("}\n");
}

void printListOfEquivalenceClasses(List *classes, PrintValue printState) {
    if (!getListSize(classes)) return;
    struct ListIterator it;
    setListIteratorNode(&it, getListHead(classes));
    do {
        printf("\t");
        printEquivalenceClass(getListIteratorValue(&it), printState);
        incListIterator(&it);
    } while (!compareListIteratorNode(&it, getListHead(classes)));
}

uint64_t findEquivalenceClassOfState(List *classes, void *state, size_t state_size) {
    struct ListIterator it;
    setListIteratorNode(&it, getListHead(classes));
    uint64_t i = 0;
    while (!containsList(getListIteratorValue(&it), state, state_size)) {
        incListIterator(&it); ++i;
    }
    return i;
}

void freeArrayOfEquivalenceClasses(List **classes, uint64_t num_of_classes, FreeValueFunction deep_free) {
    if (deep_free)
        for (uint64_t i = 0; i < num_of_classes; ++i) {
            deepClearList(classes[i], deep_free);
            free(classes[i]);
        }
    else
        for (uint64_t i = 0; i < num_of_classes; ++i) {
            clearList(classes[i]);
            free(classes[i]);
        }
    free(classes);
}

int arrayToListEquivalenceClasses(
    List *list, List **array, uint64_t num_of_classes
) {
    initList(list);
    for (uint64_t i = 0; i < num_of_classes; ++i) {
        if (
            getListSize(array[i]) && 
            pushList(list, array[i], sizeof(List))
        ) {
            deepClearList(list, (FreeValueFunction)clearList);
            freeArrayOfEquivalenceClasses(array + i, num_of_classes - i, NULL);
            return -1;
        }
        free(array[i]);
    }
    free(array);
    return 0; 
}

List **initArrayOfEquivalenceClasses(uint64_t num_of_classes) {
    List **classes = malloc(num_of_classes * sizeof(List *));
    if (!classes) return NULL;
    for (uint64_t i = 0; i < num_of_classes; ++i) {
        List *list = malloc(sizeof(List));
        if (!list) {
            freeArrayOfEquivalenceClasses(classes, i, NULL);
            return NULL;
        }
        initList(list);
        classes[i] = list;
    }
    return classes;
}

void clearListOfEquivalenceClasses(List *classes, FreeValueFunction freeValue) {
    if (!freeValue) deepClearList(classes, (FreeValueFunction)clearList);
    else while (getListSize(classes)) {
        List *list = topList(classes, 0);
        deepClearList(list, freeValue);
        free(list);
    }
}