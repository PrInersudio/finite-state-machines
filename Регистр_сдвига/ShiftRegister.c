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

static Set **initIOSets(struct ShiftRegister *reg, uint64_t upper_bound) {
    Set **sets = newIOSets((uint64_t)1 << reg->length);
    if (!sets) return NULL;
    for (uint32_t state = 0; state <= ((uint32_t)1 << reg->length) - 1; ++state)
        for (uint8_t x = 0; x < 2; ++x) {
            struct IOTuple io;
            if (initIOTuple(&io, upper_bound)) {
                freeIOSets(sets, (uint64_t)1 << reg->length, 1);
                return NULL;
            }
            pushIOTuple(&io, x, getOutputFunctionValue(reg, state, x));
            if (pushSet(sets[getStateFunctionValue(reg, state, x)], &io)) {
                freeIOTuple(&io);
                freeIOSets(sets, (uint64_t)1 << reg->length, 1);
                return NULL;
            }
        }
    return sets;
}

struct ThreadDataUpdateIOSets {
    struct ShiftRegister *reg;
    Set **sets;
    Set **new_sets;
    uint32_t start_state;
    uint32_t end_state;
    atomic_int *error_flag;
    pthread_mutex_t *mutex;
};

static void* updateIOSetsThread(void *arg) {
    struct ThreadDataUpdateIOSets *data = arg;
    for (
        uint32_t state = data->start_state;
        state <= data->end_state - 1 && atomic_load(data->error_flag) == 0;
        ++state
    ) {
        struct SetIterator it;
        for (
            initSetIterator(data->sets[state], &it);
            !reachedEndSetIterator(&it) && atomic_load(data->error_flag) == 0;
            incSetIterator(&it)
        ) {
            struct IOTuple *io[2];
            io[0] = getSetIteratorValue(&it);
            struct IOTuple io1;
            if (copyIOTuple(&io1, io[0])) {
                atomic_store(data->error_flag, -1);
                break;
            }
            io[1] = &io1;
            for (uint8_t x = 0; x < 2 && atomic_load(data->error_flag) == 0; ++x) {
                pushIOTuple(io[x], x, getOutputFunctionValue(data->reg, state, x));
                pthread_mutex_lock(data->mutex);
                if (pushSet(data->new_sets[getStateFunctionValue(data->reg, state, x)], io[x])) {
                    freeIOTuple(&io1);
                    atomic_store(data->error_flag, -1);
                    break;
                }
                pthread_mutex_unlock(data->mutex);
            }
        }
    }
    return NULL;
}

static Set **updateIOSets(struct ShiftRegister *reg, Set **sets) {
    uint64_t num_states = (uint64_t)1 << reg->length;
    uint64_t NUM_THREADS = num_states < MAX_NUM_THREADS ? num_states : MAX_NUM_THREADS;
    Set **new_sets = newIOSets(num_states);
    if (!new_sets) return NULL;
    pthread_t threads[NUM_THREADS];
    struct ThreadDataUpdateIOSets thread_data[NUM_THREADS];
    atomic_int error_flag = ATOMIC_VAR_INIT(0);
    pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
    uint32_t chunk_size = num_states / NUM_THREADS;
    uint32_t remaining = num_states % NUM_THREADS;
    uint32_t start_state = 0;
    for (int t = 0; t < NUM_THREADS; ++t) {
        uint32_t end_state = start_state + chunk_size + (t < remaining ? 1 : 0);
        thread_data[t].reg = reg;
        thread_data[t].sets = sets;
        thread_data[t].new_sets = new_sets;
        thread_data[t].start_state = start_state;
        thread_data[t].end_state = end_state;
        thread_data[t].error_flag = &error_flag;
        thread_data[t].mutex = &mutex;
        pthread_create(&threads[t], NULL, updateIOSetsThread, &thread_data[t]);
        start_state = end_state;
    }
    for (int t = 0; t < NUM_THREADS; ++t) {
        pthread_join(threads[t], NULL);
    }
    pthread_mutex_destroy(&mutex);
    if (atomic_load(&error_flag)) {
        freeIOSets(new_sets, num_states, 0);
        freeIOSets(sets, num_states, 1);
        return NULL;
    }
    freeIOSets(sets, num_states, 0);
    return new_sets;
}

static int countMemory(
    struct ShiftRegister *reg,
    uint64_t upper_bound,
    Set ***sets,
    uint64_t *memory_size
) {
    for (*memory_size = 1; *memory_size <= upper_bound; ++(*memory_size)) {
        fprintf(stderr, "countMemory memory_size = %" PRIu64 "\n", *memory_size);
        //printIOSets(*sets, (uint64_t)1 << reg->length);
        int criteria = checkMemoryCriteria(*sets, (uint64_t)1 << reg->length);
        if (criteria < 0) {
            freeIOSets(*sets, (uint64_t)1 << reg->length, 1);
            return -1;
        }
        if (!(*sets = updateIOSets(reg, *sets))) return -2;
        if (criteria == 1) break;
    }
    return 0;
}

int getMemoryShiftRegister(struct Memory* memory, struct ShiftRegister *reg) {
    uint64_t upper_bound;
    uint64_t memory_size = 0;
    int rc = 0;
    if (getMemoryUpperBound(reg, &upper_bound)) return -1;
    Set **sets = initIOSets(reg, upper_bound + 1);
    if (!sets) return -2;
    if (upper_bound != 0)
        if (countMemory(reg, upper_bound, &sets, &memory_size)) return -3;
    rc = initMemory(
        memory, sets, (uint64_t)1 << reg->length, memory_size, memory_size > upper_bound
    ) ? -4 : 0;
    freeIOSets(sets, (uint64_t)1 << reg->length, rc != 0);
    return rc;
}