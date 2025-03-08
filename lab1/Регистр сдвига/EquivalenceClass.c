#include "EquivalenceClass.h"
#include <stdlib.h>
#include <stdio.h>

void printEquivalenceClass(List *class) {
    printf("{ ");
    if (getListSize(class)) {
        struct ListIterator it;
        setListIteratorNode(&it, getListHead(class));
        do {
            printf("%u ", *(uint32_t *)getListIteratorValue(&it));
            incListIterator(&it);
        } while (!compareListIteratorNode(&it, getListHead(class)));
    }
    printf("}\n");
}

void printListOfEquivalenceClasses(List *classes) {
    if (!getListSize(classes)) return;
    struct ListIterator it;
    setListIteratorNode(&it, getListHead(classes));
    do {
        printf("\t");
        printEquivalenceClass(getListIteratorValue(&it));
        incListIterator(&it);
    } while (!compareListIteratorNode(&it, getListHead(classes)));
}

uint64_t findEquivalenceClassOfState(List *classes, uint32_t state) {
    struct ListIterator it;
    setListIteratorNode(&it, getListHead(classes));
    uint64_t i = 0;
    while (!containsList(getListIteratorValue(&it), &state, sizeof(uint32_t))) {
        incListIterator(&it); ++i;
    }
    return i;
}

void freeArrayOfEquivalenceClasses(List **classes, uint64_t num_of_classes) {
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
            freeArrayOfEquivalenceClasses(array + i, num_of_classes - i);
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
            freeArrayOfEquivalenceClasses(classes, i);
            return NULL;
        }
        initList(list);
        classes[i] = list;
    }
    return classes;
}