#ifndef MINIMIZED_H
#define MINIMIZED_H

#include <stdint.h>
#include "EquivalenceClass.h"

struct Minimized {
    List *equivalence_classes;
    uint8_t original_is_minimal;
    uint64_t degree_of_distinguishability;
    PrintState printState;
    FreeValueFunction freeValue;
};

void freeMinimized(struct Minimized *minimized);
void printMinimized(struct Minimized *minimized);

#endif