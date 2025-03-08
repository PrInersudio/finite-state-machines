#ifndef EQUIVALENCE_CLASS_H
#define EQUIVALENCE_CLASS_H

#include <stdint.h>
#include "List.h"

void printEquivalenceClass(List *class);
void printListOfEquivalenceClasses(List *classes);
uint64_t findEquivalenceClassOfState(List *classes, uint32_t state);
void freeArrayOfEquivalenceClasses(List **classes, uint64_t num_of_classes);
int arrayToListEquivalenceClasses(
    List *list, List **array, uint64_t num_of_classes
);
List **initArrayOfEquivalenceClasses(uint64_t num_of_classes);


#endif