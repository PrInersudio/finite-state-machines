#include "EquivalenceClass.h"
#include <stdlib.h>
#include <stdio.h>

void printEquivalenceClass(List *class) {
    printf("{ ");
    struct ListIterator it;
    initListIterator(class, &it);
    for (uint64_t i = 0; i < getListSize(class); ++i) {
        printf("%u ", *((uint32_t *)getListIteratorValue(&it)));
        incListIterator(&it);
    }
    printf("}\n");
}

void printListOfEquivalenceClasses(List *classes) {
    struct ListIterator it;
    initListIterator(classes, &it);
    for (uint64_t i = 0; i < getListSize(classes); ++i) {
        printf("\t");
        printEquivalenceClass(getListIteratorValue(&it));
        incListIterator(&it);
    }
}

void freeEquivalenceClass(List *class, uint8_t free_states) {
    if (free_states) {
        while(getListSize(class))
            free(topList(class));
    }
    else {
        while(getListSize(class))
            topList(class);
    }
}

void freeListOfEquivalenceClasses(List *classes, uint8_t free_states) {
    while(getListSize(classes)) {
        List *list = topList(classes);
        freeEquivalenceClass(list, free_states);
        free(list);
    }       
}

uint64_t findEquivalenceClassOfState(List *classes, uint32_t state) {
    struct ListIterator it;
    initListIterator(classes, &it);
    uint64_t i;
    for (i = 0; i < getListSize(classes); ++i) {
        if (containsList(getListIteratorValue(&it), &state, sizeof(uint32_t)))
            break;
        incListIterator(&it);
    }
    return i;
}

void freeArrayOfEquivalenceClasses(List **classes, uint64_t num_of_classes, uint8_t free_states) {
    if (free_states) {
        for (uint64_t i = 0; i < num_of_classes; ++i)
            freeEquivalenceClass(classes[i], 1);
    }
    for (uint64_t i = 0; i < num_of_classes; ++i) {
        if (classes[i]) {
            freeList(classes[i]);
            free(classes[i]);
        }
    }
    free(classes);
}

int arrayToListEquivalenceClasses(
    List *list, List **array, uint64_t num_of_classes,
    uint8_t free_states_in_case_of_error
) {
    initList(list);
    for (uint64_t i = 0; i < num_of_classes; ++i) {
        if (getListSize(array[i])) {
            if (pushList(list, array[i])) {
                freeListOfEquivalenceClasses(list, free_states_in_case_of_error);
                return -1;
            }
        }
        else {
            free(array[i]);
            array[i] = NULL;
        }
    }
    return 0;
}

List **initArrayOfEquivalenceClasses(uint64_t num_of_classes) {
    List **classes = (List **)malloc(num_of_classes * sizeof(List *));
    if (!classes) return NULL;
    for (uint64_t i = 0; i < num_of_classes; ++i) {
        List *list = (List *)malloc(sizeof(List));
        if (!list) {
            freeArrayOfEquivalenceClasses(classes, i, 0);
            return NULL;
        }
        initList(list);
        classes[i] = list;
    }
    return classes;
}