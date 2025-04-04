#include "ShiftRegister.h"
#include <inttypes.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdatomic.h>

int initShiftRegisterFromFile(struct ShiftRegister* reg, char* settings_file) {
    FILE *fp = fopen(settings_file, "r");
    if (!fp) {
        printf("Не открывается файл %s\n", settings_file);
        return -1;
    }
    int rc = 0;
    char buf[3];
    fgets(buf, 3, fp);
    unsigned temp;
    if (sscanf(buf, "%u", &temp) != 1) {
        printf("Не считывается размер регистра \n");
        rc = -2;
        goto end;
    }
    reg->length = (uint8_t)temp;
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

uint32_t getState(struct ShiftRegister* reg) {
    return reg->state;
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

static int minimizationFirstStep(struct ShiftRegister* original, List* first_step) {
    List **classes = initArrayOfEquivalenceClasses(4);
    if (!classes) return -1;
    for (
        uint32_t state = 0;
        state <= ((uint32_t)1 << original->length) - 1;
        ++state
    ) {
        uint8_t state_ouput_mask =
            (getOutputFunctionValue(original, state, 0) << 1) |
            getOutputFunctionValue(original, state, 1);
        if (pushList(classes[state_ouput_mask], &state, sizeof(uint32_t))) {
            freeArrayOfEquivalenceClasses(classes, 4, NULL);
            return -2;
        }
    }
    if (arrayToListEquivalenceClasses(first_step, classes, 4)) return -3;
    return 0;
}

static int putStateIntoNewClass(
    struct ShiftRegister* original,
    List *current_step,
    List **classes,
    uint32_t state
) {
    uint32_t next_state_0 = getStateFunctionValue(original, state, 0);
    uint32_t next_state_1 = getStateFunctionValue(original, state, 1);
    uint64_t state_transition_mask = getListSize(current_step) *
        findEquivalenceClassOfState(current_step, &next_state_0, sizeof(uint32_t)) +
        findEquivalenceClassOfState(current_step, &next_state_1, sizeof(uint32_t));
        if (pushList(classes[state_transition_mask], &state, sizeof(uint32_t))) {
            return -1;
        }
    return 0;
}

static int copyOneStateClass(List *next_step, List *class) {
    List copy;
    if (copyList(&copy, class)) return -1;
    if (pushList(next_step, &copy, sizeof(List))) {
        clearList(&copy);
        return -2;
    }
    return 0;
}

static int divideClass(
    struct ShiftRegister* original,
    List *current_step,
    List *next_step,
    List *class
) {
    uint64_t num_of_new_classes =
        getListSize(current_step) *
        getListSize(current_step);
    List **classes = initArrayOfEquivalenceClasses(num_of_new_classes);
    if (!classes) return -1;
    struct ListIterator it;
    setListIteratorNode(&it, getListHead(class));
    do {
        if (putStateIntoNewClass(original, current_step, classes, *(uint32_t *)getListIteratorValue(&it))) {
            freeArrayOfEquivalenceClasses(classes, num_of_new_classes, NULL);
            return -2;
        }
        incListIterator(&it);
    } while (!compareListIteratorNode(&it, getListHead(class)));
    List list_of_new_classes;
    if (arrayToListEquivalenceClasses(
        &list_of_new_classes, classes, num_of_new_classes
    )) return -3;
    transfer(&list_of_new_classes, next_step);
    return 0;
}

static int minimizationStep(
    struct ShiftRegister* original,
    List *current_step,
    List *next_step
) {
    struct ListIterator it;
    setListIteratorNode(&it, getListHead(current_step));
    do {
        List *class = getListIteratorValue(&it);
        if (getListSize(class) == 1) {
            if (copyOneStateClass(next_step, class)) goto err;
        } 
        else if (divideClass(original, current_step, next_step, class)) goto err;
        incListIterator(&it);
    } while (!compareListIteratorNode(&it, getListHead(current_step)));
    return 0;
err:
    deepClearList(next_step, (FreeValueFunction)clearList);
    return -1;
}

void printState(uint32_t *state) {
    printf("%" PRIu32 "", *state);
}

int minimizeShiftRegister(
    struct Minimized *minimized,
    struct ShiftRegister* original
) {
    List *current_step = malloc(sizeof(List));
    if (!current_step) return -1;
    if (minimizationFirstStep(original, current_step)) {
        free(current_step);
        return -2;
    }
    if (getListSize(current_step) == 1) {
        minimized->equivalence_classes = current_step;
        minimized->degree_of_distinguishability = 0;
        minimized->original_is_minimal = 0;
        minimized->printState = (PrintValue)printState;
        minimized->freeValue = NULL;
        return 0;
    }
    int rc = 0;
    List *next_step = malloc(sizeof(List));
    if (!next_step) {
        rc = -4;
        goto end;
    }
    initList(next_step);
    uint64_t degree_of_distinguishability = 0;
    while(1)  {
        ++degree_of_distinguishability;
        printf("Классы %" PRIu64 " эквивалентности:\n", degree_of_distinguishability);
        printListOfEquivalenceClasses(current_step, (PrintValue)printState);
        if (minimizationStep(original, current_step, next_step)) {
            free(next_step);
            rc = -5;
            goto end;
        }
        if (getListSize(next_step) == getListSize(current_step)) break;
        deepClearList(current_step, (FreeValueFunction)clearList);
        List *temp = current_step;
        current_step = next_step;
        next_step = temp;
    }
    minimized->equivalence_classes = next_step;
    minimized->degree_of_distinguishability = degree_of_distinguishability;
    minimized->original_is_minimal =
        getListSize(minimized->equivalence_classes) ==
        (uint64_t)1 << original->length;
    minimized->printState = (PrintValue)printState;
    minimized->freeValue = NULL;
end:
    deepClearList(current_step, (FreeValueFunction)clearList);
    free(current_step);
    return rc;
}

int shiftRegisterToGraph(struct ShiftRegister *reg, struct Graph *graph) {
    if (initGraph(graph, (uint64_t)1 << reg->length)) return -1;
    for (uint32_t i = 0; i <= ((uint32_t)1 << reg->length) - 1; ++i) {
        setOrDeleteEdge(graph, i, getStateFunctionValue(reg, i, 0), 1);
        setOrDeleteEdge(graph, i, getStateFunctionValue(reg, i, 1), 1);
    }
    return 0;
}