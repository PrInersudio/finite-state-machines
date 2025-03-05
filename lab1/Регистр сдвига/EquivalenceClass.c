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

void printEquivalenceClasses(List *classes) {
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

uint8_t doesEquivalenceClassContainState(List *class, uint32_t state) {
    struct ListIterator it;
    initListIterator(class, &it);
    for (uint64_t i = 0; i < getListSize(class); ++i) {
        if (*((uint32_t *)getListIteratorValue(&it)) == state) return 1;
        incListIterator(&it);
    }
    return 0;
}

uint64_t findClassOfState(List *classes, uint32_t state) {
    struct ListIterator it;
    initListIterator(classes, &it);
    uint64_t i;
    for (i = 0; i < getListSize(classes); ++i) {
        if (
            doesEquivalenceClassContainState(
                getListIteratorValue(&it), state
            )
        ) break;
        incListIterator(&it);
    }
    return i;
}

void freeArrayOfClasses(List **classes, uint64_t num_of_classes) {
    for (uint64_t i = 0; i < num_of_classes; ++i) {
        if (classes[i]) {
            freeList(classes[i]);
            free(classes[i]);
        }
    }
    free(classes);
}

void freeArrayOfClassesWithStates(List **classes, uint64_t num_of_classes) {
    for (uint64_t i = 0; i < num_of_classes; ++i) freeEquivalenceClass(classes[i], 1);
    freeArrayOfClasses(classes, num_of_classes);
}

int arrayOfClassesToList(
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

List **initArrayOfClasses(uint64_t num_of_classes) {
    List **classes = (List **)malloc(num_of_classes * sizeof(List *));
    if (!classes) return NULL;
    for (uint64_t i = 0; i < num_of_classes; ++i) {
        List *list = (List *)malloc(sizeof(List));
        if (!list) {
            freeArrayOfClasses(classes, i);
            return NULL;
        }
        initList(list);
        classes[i] = list;
    }
    return classes;
}