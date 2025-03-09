#include "Minimized.h"
#include <stdlib.h>
#include <stdio.h>
#include <inttypes.h>

void freeMinimized(struct Minimized *minimized) {
    clearListOfEquivalenceClasses(
        minimized->equivalence_classes,
        minimized->freeValue
    );
    free(minimized->equivalence_classes);
    minimized->degree_of_distinguishability = 0;
    minimized->original_is_minimal = 0;
}

void printMinimized(struct Minimized *minimized) {
    printf("Классы эквивалентности:\n");
    printListOfEquivalenceClasses(
        minimized->equivalence_classes,
        minimized->printState
    );
    printf("Приведённый вес: %" PRIu64 "\n", getListSize(minimized->equivalence_classes));
    printf("Степень различимости: %" PRIu64 "\n", minimized->degree_of_distinguishability);
    printf("Минимальный? ");
    if (minimized->original_is_minimal) printf("Да\n");
    else printf("Нет\n");
}