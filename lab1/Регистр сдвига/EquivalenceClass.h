#ifndef EQUIVALENCE_CLASS_H
#define EQUIVALENCE_CLASS_H

#include <stdint.h>
#include "List.h"

void printEquivalenceClass(List *class);
void printEquivalenceClasses(List *classes);
void freeEquivalenceClass(List *class, uint8_t free_states);
void freeListOfEquivalenceClasses(List *classes, uint8_t free_states);
uint8_t doesEquivalenceClassContainState(List *class, uint32_t state);
uint64_t findClassOfState(List *classes, uint32_t state);
void freeArrayOfClasses(List **classes, uint64_t num_of_classes);
void freeArrayOfClassesWithStates(List **classes, uint64_t num_of_classes);
int arrayOfClassesToList(
    List *list, List **array, uint64_t num_of_classes,
    uint8_t free_states_in_case_of_error
);
List **initArrayOfClasses(uint64_t num_of_classes);


#endif