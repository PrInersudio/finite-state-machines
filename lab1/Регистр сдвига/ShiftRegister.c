#include "ShiftRegister.h"
#include <stdlib.h>

int initShiftRegisterFromFile(struct ShiftRegister* reg, char* settings_file) {
    FILE *fp = fopen(settings_file, "r");
    if (fp == NULL) return -1;
    int rc = 0;
    char buf[3];
    fgets(buf, 3, fp);
    if (sscanf(buf, "%u", &reg->length) != 1) {
        rc = -2;
        goto end;
    }
    if (reg->length > 32) {
        rc = -3;
        goto end;
    }
    reg->mask = (1 << reg->length) - 1;
    if (initBitArray(&reg->phi, (uint64_t)1 << (reg->length + 1))) {
        rc = -4;
        goto end;
    }
    if (readArrayFromFile(&reg->phi, (uint64_t)1 << (reg->length + 1), fp)) {
        freeBitArray(&reg->phi);
        rc = -5;
        goto end;
    }
    if (initBitArray(&reg->psi, (uint64_t)1 << (reg->length + 1))) {
        freeBitArray(&reg->phi);
        rc = -6;
        goto end;
    }
    if (readArrayFromFile(&reg->psi, (uint64_t)1 << (reg->length + 1), fp)) {
        freeBitArray(&reg->phi);
        freeBitArray(&reg->psi);
        rc = -7;
        goto end;
    }
end:
    fclose(fp);
    return rc;
}

int readState(struct ShiftRegister* reg) {
    printf("Введите начальное состояние: ");
    reg->state = 0;
    for (uint8_t i = 0; i < reg->length; ++i) {
        switch (fgetc(stdin)) {
            case '1':
                reg->state = (reg->state << 1) | 1;
                break;
            case '0':
                reg->state = (reg->state << 1);
                break;
            case ' ':
            case '\t':
            case '\n':
                --i;
                break;
            default:
                return -1;
        }
    }
    return 0;
}

uint8_t useShiftRegister(struct ShiftRegister* reg, uint8_t x) {
    reg->state <<= 1;
    uint8_t phi = getBitArrayElement(&reg->phi, reg->state | x);
    uint8_t y = getBitArrayElement(&reg->psi, reg->state | phi);
    reg->state = (reg->state | phi) & reg->mask;
    return y;
}

void freeShiftRegister(struct ShiftRegister* reg) {
    reg->length = 0;
    freeBitArray(&reg->phi);
    freeBitArray(&reg->psi);
}

static uint32_t getStateFunctionValue(struct ShiftRegister* reg, uint32_t state, uint8_t x) {
    state <<= 1;
    uint8_t phi = getBitArrayElement(&reg->phi, state | x);
    return (state | phi) & reg->mask;
}

static uint8_t getOutputFunctionValue(struct ShiftRegister* reg, uint32_t state, uint8_t x) {
    state <<= 1;
    uint8_t phi = getBitArrayElement(&reg->phi, state | x);
    return getBitArrayElement(&reg->psi, state | phi);
}

static List **minimizationFirstStep(struct ShiftRegister* original) {
    List **classes = initArrayOfClasses(4);
    if (!classes) return NULL;
    for (uint32_t i = 0; i <= ((uint32_t)1 << original->length) - 1; ++i) {
        uint32_t *state = (uint32_t *)malloc(sizeof(uint32_t));
        if (!state) goto err;
        *state = i;
        uint8_t state_ouput_mask = (getOutputFunctionValue(original, *state, 0) << 1) |
            getOutputFunctionValue(original, *state, 1);
        if (pushList(classes[state_ouput_mask], state)) {
            free(state);
            goto err;
        }
    }
    return classes;
err:
    freeArrayOfClassesWithStates(classes, 4);
    return NULL;
}

static int divideClass(
    struct ShiftRegister* original,
    List *current_step,
    List *next_step,
    List *class
) {
    //printEquivalenceClass(class);
    uint64_t num_of_new_classes = getListSize(current_step) * getListSize(current_step);
    List **classes = initArrayOfClasses(num_of_new_classes);
    if (!classes) return -1;
    struct ListIterator it;
    initListIterator(class, &it);
    for (uint64_t i = 0; i < getListSize(class); ++i) {
        uint32_t *state = getListIteratorValue(&it);
        uint64_t state_transition_mask =
            findClassOfState(current_step, getStateFunctionValue(original, *state, 0)) * getListSize(current_step) +
            findClassOfState(current_step, getStateFunctionValue(original, *state, 1));
        if (pushList(classes[state_transition_mask], state)) goto err;
        incListIterator(&it);
    }
    List list_of_new_classes;
    if (arrayOfClassesToList(&list_of_new_classes, classes, num_of_new_classes, 0)) goto err;
    free(classes);
    transfer(&list_of_new_classes, next_step);
    return 0;
err:
    freeArrayOfClasses(classes, num_of_new_classes);
    return -2;
}

static int minimizationStep(
    struct ShiftRegister* original,
    List *current_step,
    List *next_step
) {
    initList(next_step);
    struct ListIterator it;
    initListIterator(current_step, &it);
    for (uint64_t i = 0; i < getListSize(current_step); ++i) {
        List *class = getListIteratorValue(&it);
        if (getListSize(current_step) == 1) {
            if (pushList(next_step, class)) goto err;
        }
        else {
            if (divideClass(original, current_step, next_step, class)) goto err;
        }
        incListIterator(&it);
    }
    return 0;
err:
    freeListOfEquivalenceClasses(next_step, 0);
    return -1;
}

int minimizeShiftRegister(
    struct MinimizedShiftRegister *minimized,
    struct ShiftRegister* original
) {
    List *current_step = (List *)malloc(sizeof(List));
    if (!current_step) return -1;
    List **first_step = minimizationFirstStep(original);
    if (!first_step) {
        free(current_step);
        return -2;
    }
    if (arrayOfClassesToList(current_step, first_step, 4, 1)) {
        free(current_step);
        freeArrayOfClassesWithStates(first_step, 4);
        return -3;
    }
    free(first_step);
    if (getListSize(current_step) == 1) {
        minimized->equivalence_classes = current_step;
        minimized->degree_of_distinguishability = 0;
        minimized->original_is_minimal = 0;
        return 0;
    }
    int rc = 0;
    List *next_step = (List *)malloc(sizeof(List));
    if (!next_step) {
        rc = -4;
        goto end;
    }
    uint64_t degree_of_distinguishability = 0;
    while(1)  {
        ++degree_of_distinguishability;
        printf("Классы %u эквивалентности:\n", degree_of_distinguishability);
        printEquivalenceClasses(current_step);
        if (minimizationStep(original, current_step, next_step)) {
            free(next_step);
            rc = -5;
            goto end;
        }
        if (getListSize(next_step) == getListSize(current_step)) break;
        freeListOfEquivalenceClasses(current_step, 0);
        List *temp = current_step;
        current_step = next_step;
        next_step = temp;
    }
    minimized->equivalence_classes = next_step;
    minimized->degree_of_distinguishability = degree_of_distinguishability;
    minimized->original_is_minimal =
        getListSize(minimized->equivalence_classes) ==
        (uint64_t)1 << original->length;
end:
    freeListOfEquivalenceClasses(current_step, rc != 0);
    free(current_step);
    return rc;
}

void freeMinimizedShiftRegister(struct MinimizedShiftRegister *minimized) {
    freeListOfEquivalenceClasses(minimized->equivalence_classes, 1);
    free(minimized->equivalence_classes);
    minimized->degree_of_distinguishability = 0;
    minimized->original_is_minimal = 0;
}

void printMinimizedShiftRegister(struct MinimizedShiftRegister *minimized) {
    printf("Классы эквивалентности:\n");
    printEquivalenceClasses(minimized->equivalence_classes);
    printf("Приведённый вес: %llu\n", getListSize(minimized->equivalence_classes));
    printf("Степень различимости: %llu\n", minimized->degree_of_distinguishability);
    printf("Минимальный? ");
    if (minimized->original_is_minimal) printf("Да\n");
    else printf("Нет\n");
}