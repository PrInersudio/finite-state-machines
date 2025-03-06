#ifndef EQUIVALENCE_CLASS_H
#define EQUIVALENCE_CLASS_H

#include <stdint.h>
#include "List.h"

void printEquivalenceClass(List *class);
void printListOfEquivalenceClasses(List *classes);
void freeEquivalenceClass(List *class, uint8_t free_states);
void freeListOfEquivalenceClasses(List *classes, uint8_t free_states);
uint64_t findEquivalenceClassOfState(List *classes, uint32_t state);
void freeArrayOfEquivalenceClasses(List **classes, uint64_t num_of_classes, uint8_t free_states);
int arrayToListEquivalenceClasses(
    List *list, List **array, uint64_t num_of_classes,
    uint8_t free_states_in_case_of_error
);
List **initArrayOfEquivalenceClasses(uint64_t num_of_classes);


#endif