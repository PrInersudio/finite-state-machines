#include "ShiftRegister.h"
#include <inttypes.h>
#include <stdlib.h>
#include "Memory.h"

int initShiftRegisterFromFile(struct ShiftRegister* reg, char* settings_file) {
    FILE *fp = fopen(settings_file, "r");
    if (fp == NULL) return -1;
    int rc = 0;
    char buf[3];
    fgets(buf, 3, fp);
    unsigned temp;
    if (sscanf(buf, "%u", &temp) != 1) {
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

void printGF2(uint8_t *element) {
    printf("%" PRIu8 "", *element);
}

static struct Trace *initTrace(struct ShiftRegister *reg, uint32_t state, uint8_t input) {
    struct Trace *trace = newTrace(
        sizeof(uint8_t), sizeof(uint8_t),
        NULL, NULL, NULL, NULL, NULL, NULL, (PrintValue)printGF2, (PrintValue)printGF2, NULL
    );
    if (!trace) return NULL;
    uint32_t new_state = getStateFunctionValue(reg, state, input);
    uint8_t output = getOutputFunctionValue(reg, state, input);
    if (
        setTraceFinalState(trace, &new_state, sizeof(uint8_t), 0) ||
        pushInputTrace(trace, &input) || pushOutputTrace(trace, &output)
    ) {
        freeTrace(trace);
        free(trace);
    };
    return trace;
}

static List **initTraces(struct ShiftRegister *reg) {
    List **traces = newTraces((uint64_t)1 << reg->length);
    if (!traces) return NULL;
    for (uint32_t state = 0; state <= ((uint32_t)1 << reg->length) - 1; ++state)
        for (uint8_t input = 0; input < 2; ++input) {
            struct Trace *trace = initTrace(reg, state, input);
            if (!trace) goto err;
            if (pushList(traces[state], trace, sizeof(struct Trace))) {
                freeTrace(trace);
                free(trace);
                goto err;
            }
        }
    return traces;
err:
    freeTraces(traces, (uint64_t)1 << reg->length);
    return NULL;
}

static struct Trace *updateTrace(
    struct ShiftRegister *reg, struct Trace *old,
    uint32_t state, uint8_t input
) {
    struct Trace *new = copyTrace(old);
    if (!new) return NULL;
    uint8_t output = getOutputFunctionValue(reg, state, input);
    if (pushInputTrace(new, &input) || pushOutputTrace(new, &output)) {
        freeTrace(new);
        free(new);
        return NULL;
    }
    return new;          
}

List **updateTraces(struct ShiftRegister *reg, List **traces) {
    uint8_t err = 0;
    List **new_traces = newTraces((uint64_t)1 << reg->length);
    if (!new_traces) goto end;
    for (uint32_t state = 0; state <= ((uint32_t)1 << reg->length) - 1; ++state)
        for (uint8_t input = 0; input < 2; ++input) {
            uint32_t next_state = getStateFunctionValue(reg, state, input);
            if (!getListSize(traces[next_state])) continue;
            struct ListIterator it;
            setListIteratorNode(&it, getListHead(traces[next_state]));
            do {
                struct Trace *trace = updateTrace(reg, getListIteratorValue(&it), state, input);
                if (!trace) { err = 1; goto end; }
                if (pushList(new_traces[state], trace, sizeof(struct Trace))) {
                    freeTrace(trace);
                    free(trace);
                    err = 1; goto end;
                }
            } while (!compareListIteratorNode(&it, getListHead(traces[next_state])));
        }
end:
    freeTraces(traces, (uint64_t)1 << reg->length);
    if (!err) return new_traces;
    else {
        freeTraces(new_traces, (uint64_t)1 << reg->length);
        return NULL;
    }
}

Set **getSetsOfIOTuplesFromTraces(List **traces, uint64_t num_of_states) {
    Set **sets = newSetsOfIOTuples(num_of_states);
    if (sets) return NULL;
    for (uint32_t state = 0; state <= (uint32_t)(num_of_states - 1); ++state) {
        if (!getListSize(traces[state])) continue;
        struct ListIterator it;
        setListIteratorNode(&it, getListHead(traces[state]));
        do {
            struct Trace *trace = getListIteratorValue(&it);
            if (pushSet(sets[*(uint32_t *)getTraceFinalState(trace)], getTraceIOTuple(trace))) {
                freeSetsOfIOTuples(sets, num_of_states, 0);
                return NULL;
            }
        } while (!compareListIteratorNode(&it, getListHead(traces[state])));
    }
    return sets;
}

static int getMemoryUpperBound(struct ShiftRegister *reg, uint64_t *upper_bound) {
    struct Minimized minimized;
    if (minimizeShiftRegister(&minimized, reg)) return -1;
    *upper_bound = (
        getListSize(minimized.equivalence_classes) *
        (getListSize(minimized.equivalence_classes) - 1)
    ) >> 1;
    freeMinimized(&minimized);
    return 0;
}

static int initMemoryFromTraces(
    struct ShiftRegister *reg,
    struct Memory* memory, 
    List **traces,
    uint64_t m, uint8_t infinite
) {
    if (infinite) {
        memory->infinite = 1;
        return 0;
    }
    memory->infinite = 0;
    memory->m = m;
    memory->memory_table = malloc(((size_t)1 << (2 * m + 1)) * sizeof(struct IOTuple *));
    if (!memory->memory_table) return -1;
    memory->memory_table_size = 0;
    for (uint32_t state = 0; state <= (uint32_t)(((uint32_t)1 << reg->length) - 1 - 1); ++state)
        while (getListSize(traces[state])) {
            struct Trace *trace = topList(traces[state], 0);
            struct IOTuple *io[2];
            io[0] = popTraceIOTuple(trace);
            io[1] = copyIOTuple(io[0]);
            if (!io[1]) {
                freeTrace(trace); free(trace);
                freeIOTuple(io[0]); free(io[0]);
                freeMemory(memory);
                return -2;
            }
            uint32_t *final_state = getTraceFinalState(trace);
            for (uint8_t input = 0; input < 2; ++input) {
                uint8_t output = getOutputFunctionValue(reg, *final_state, input);
                if (
                    pushBackInputIOTuple(io[input], &input) ||
                    pushBackInputIOTuple(io[input], &output)
                ) {
                        freeTrace(trace); free(trace);
                        for (uint8_t j = input; j < 2; ++j) { freeIOTuple(io[j]); free(io[j]); }
                        freeMemory(memory);
                        return -3;
                }
                ++memory->memory_table_size;
            }
            freeTrace(trace); free(trace);
        }
    return 0;
}

int getMemoryShiftRegister(struct Memory* memory, struct ShiftRegister *reg) {
    uint64_t upper_bound;
    if (getMemoryUpperBound(reg, &upper_bound)) return -1;
    List **traces = initTraces(reg);
    if (!traces) return -2;
    uint64_t m;
    int rc = 0;
    uint64_t num_of_states = (uint64_t)1 << reg->length;
    for (m = 0; m <= upper_bound; ++m) {
        Set **sets = getSetsOfIOTuplesFromTraces(traces, num_of_states);
        if (!sets) { rc = -3; goto end; }
        int criteria = checkMemoryCriteria(sets, num_of_states);
        freeSetsOfIOTuples(sets, num_of_states, 0);
        if (criteria < 0) { rc = -4; goto end; }
        if (criteria == 1) break;
        traces = updateTraces(reg, traces);
        if (!traces) return -5;
    }
    if (initMemoryFromTraces(reg, memory, traces, m, (m > upper_bound)))
        rc = -5;
end:
    freeTraces(traces, num_of_states);
    return rc;
}