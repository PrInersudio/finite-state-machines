#include "ShiftRegister.h"
#include <inttypes.h>
#include <stdlib.h>

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

static int initTraceShiftRegister(
    struct ShiftRegister *reg,
    uint32_t state, uint8_t input,
    List *state_traces, uint64_t upper_bound
) {
    struct Trace trace;
    if (initTrace(&trace, upper_bound)) return -1;
    updateTrace(
        &trace, input, 
        getOutputFunctionValue(reg, state, input),
        getStateFunctionValue(reg, state, input)
    );
    if (pushList(state_traces, &trace, sizeof(struct Trace))) {
        freeTrace(&trace);
        return -1;
    }
    return 0;
}

static List **initTracesShiftRegister(struct ShiftRegister *reg, uint64_t upper_bound) {
    List **traces = newTraces((uint64_t)1 << reg->length);
    if (!traces) return NULL;
    for (uint32_t state = 0; state <= ((uint32_t)1 << reg->length) - 1; ++state)
        for (uint8_t input = 0; input < 2; ++input) {
            if (initTraceShiftRegister(
                reg, state, input, traces[state], upper_bound
            )) goto err;
        }
    return traces;
err:
    freeTraces(traces, (uint64_t)1 << reg->length);
    return NULL;
}

static int updateTraces(struct ShiftRegister *reg, List **traces) {
    for (uint32_t state = 0; state <= ((uint32_t)1 << reg->length) - 1; ++state) {
        if (!getListSize(traces[state])) continue;
        List new_traces;
        initList(&new_traces);
        struct ListIterator it;
        setListIteratorNode(&it, getListHead(traces[state]));
        do {
            struct Trace *trace1 = getListIteratorValue(&it);
            struct Trace trace2;
            struct Trace *current_traces[2];
            if (copyTrace(&trace2, trace1)) return -1;
            current_traces[0] = trace1;
            current_traces[1] = &trace2;
            for (uint8_t input = 0; input < 2; ++input)
                updateTrace(
                    current_traces[input], input,
                    getOutputFunctionValue(reg, getTraceFinalState(current_traces[input]), input),
                    getStateFunctionValue(reg, getTraceFinalState(current_traces[input]), input)
                );
            if (pushList(&new_traces, &trace2, sizeof(struct Trace))) {
                freeTrace(&trace2);
                return -2;
            }
            incListIterator(&it);
        } while (!compareListIteratorNode(&it, getListHead(traces[state])));
        transfer(&new_traces, traces[state]);
    }
    return 0;
}

static Set **getSetsOfIOTuplesFromTraces(List **traces, uint64_t num_of_states) {
    Set **sets = newSetsOfIOTuples(num_of_states);
    if (!sets) return NULL;
    for (uint32_t state = 0; state <= (uint32_t)(num_of_states - 1); ++state) {
        if (!getListSize(traces[state])) continue;
        struct ListIterator it;
        setListIteratorNode(&it, getListHead(traces[state]));
        do {
            struct Trace *trace = getListIteratorValue(&it);
            if (pushSet(sets[getTraceFinalState(trace)], getTraceIOTuple(trace))) {
                freeSetsOfIOTuples(sets, num_of_states);
                return NULL;
            }
            incListIterator(&it);
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

static void traceToMemory(
    struct ShiftRegister *reg,
    struct Memory* memory,
    struct Trace *trace
) {
    struct IOTuple *io = popTraceIOTuple(trace);
    uint64_t index = getMemoryTableIndexFromIOTuple(io);
    memory->memory_table[index] = io;
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
    memory->memory_table_size = (uint64_t)1 << (2 * m + 1);
    memory->memory_table = malloc(memory->memory_table_size * sizeof(struct IOTuple *));
    if (!memory->memory_table) return -1;
    for (uint32_t state = 0; state <= (uint32_t)(((uint32_t)1 << reg->length) - 1 - 1); ++state)
        while (getListSize(traces[state])) {
            struct Trace *trace = topList(traces[state], 0);
            traceToMemory(reg, memory, trace);
            freeTrace(trace); free(trace);
        }
    return 0;
}

int getMemoryShiftRegister(struct Memory* memory, struct ShiftRegister *reg) {
    uint64_t upper_bound;
    if (getMemoryUpperBound(reg, &upper_bound)) return -1;
    List **traces = initTracesShiftRegister(reg, upper_bound + 1);
    if (!traces) return -2;
    uint64_t m;
    int rc = 0;
    uint64_t num_of_states = (uint64_t)1 << reg->length;
    for (m = 0; m <= upper_bound; ++m) {
        fprintf(stderr, "%" PRIu64 "\n", m);
        Set **sets = getSetsOfIOTuplesFromTraces(traces, num_of_states);
        //printSetsOfIOTuples(sets, num_of_states);
        if (!sets) { rc = -3; goto end; }
        int criteria = checkMemoryCriteria(sets, num_of_states);
        freeSetsOfIOTuples(sets, num_of_states);
        if (criteria < 0) { rc = -4; goto end; }
        if (updateTraces(reg, traces)) { rc = -5; goto end; }
        if (criteria == 1) { ++m; break; }
    }
    if (initMemoryFromTraces(reg, memory, traces, m, (m > upper_bound)))
        rc = -6;
end:
    freeTraces(traces, num_of_states);
    return rc;
}