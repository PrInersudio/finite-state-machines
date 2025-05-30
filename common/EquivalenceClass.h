#ifndef EQUIVALENCE_CLASS_H
#define EQUIVALENCE_CLASS_H

#include <stdint.h>
#include "List.h"

void printEquivalenceClass(List *class, PrintValue printState);
void printListOfEquivalenceClasses(List *classes, PrintValue print_state);
uint64_t findEquivalenceClassOfState(List *classes, void *state, size_t state_size);
void freeArrayOfEquivalenceClasses(List **classes, uint64_t num_of_classes, FreeValueFunction deep_free);
int arrayToListEquivalenceClasses(
    List *list, List **array, uint64_t num_of_classes
);
List **initArrayOfEquivalenceClasses(uint64_t num_of_classes);
void clearListOfEquivalenceClasses(List *classes, FreeValueFunction freeValue);


#endif