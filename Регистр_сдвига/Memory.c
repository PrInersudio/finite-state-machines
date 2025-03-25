#include "Memory.h"
#include <stdlib.h>
#include <inttypes.h>
#include <pthread.h>
#include <stdatomic.h>

uint64_t hashIOTuple(struct IOTuple *io) {
    uint64_t hash = hashBitArray(io->input_sequence) * 31 + hashBitArray(io->output_sequence);
    return hash;
}

uint8_t compareIOTuples(struct IOTuple *io1, struct IOTuple *io2) {
    return compareFirstNBytesOfBitArray(io1->input_sequence, io2->input_sequence, (io1->current_length + 7) / 8) &&
        compareFirstNBytesOfBitArray(io1->output_sequence, io2->output_sequence, (io1->current_length + 7) / 8);
}

int initIOTuple(struct IOTuple *io, uint64_t upper_bound) {
    io->input_sequence = malloc(sizeof(BitArray));
    if (!io->input_sequence)  return -1;
    if(initBitArray(io->input_sequence, upper_bound)) {
        free(io->input_sequence);
        return -2;
    }
    io->output_sequence = malloc(sizeof(BitArray));
    if (!io->output_sequence) {
        free(io->input_sequence);
        return -3;
    }
    if(initBitArray(io->output_sequence, upper_bound)) {
        free(io->output_sequence);
        free(io->input_sequence);
        return -4;
    }
    io->current_length = 0;
    return 0;
}

void freeIOTuple(struct IOTuple *io) {
    freeBitArray(io->input_sequence);
    free(io->input_sequence);
    freeBitArray(io->output_sequence);
    free(io->output_sequence);
}

void pushIOTuple(struct IOTuple *io, uint8_t input, uint8_t output) {
    setBitArrayElement(io->input_sequence, io->current_length, input);
    setBitArrayElement(io->output_sequence, io->current_length, output);
    ++io->current_length;
}

int copyIOTuple(struct IOTuple *dst, struct IOTuple *src) {
    dst->input_sequence = malloc(sizeof(BitArray));
    if (!dst->input_sequence) return -1;
    if (copyBitArray(dst->input_sequence, src->input_sequence)) {
        free(dst->input_sequence);
        return -2;
    }
    dst->output_sequence = malloc(sizeof(BitArray));
    if (!dst->output_sequence) {
        return -3;
    }
    if (copyBitArray(dst->output_sequence, src->output_sequence)) {
        free(dst->output_sequence);
        freeBitArray(dst->input_sequence);
        free(dst->input_sequence);
        return -4;
    }
    dst->current_length = src->current_length;
    return 0;
}

void printIOTuple(struct IOTuple *io) {
    printf("Вход: ");
    for (uint64_t i = 0; i < io->current_length; ++i)
        printf("%" PRIu8 " ", getBitArrayElement(io->input_sequence, i));
    printf("Выход: ");
    for (uint64_t i = 0; i < io->current_length; ++i)
        printf("%" PRIu8 " ", getBitArrayElement(io->output_sequence, i));
}

void freeIOSets(Set **sets, uint64_t num_of_states, uint8_t deep) {
    for (uint64_t i = 0; i < num_of_states; ++i) {
        freeSet(sets[i], deep);
        free(sets[i]);
    }
    free(sets);
}

Set **newIOSets(uint64_t num_of_states) {
    Set **sets = malloc(num_of_states * sizeof(Set *));
    if (!sets) return NULL;
    uint64_t i;
    for (i = 0; i < num_of_states; ++i) {
        Set *set = malloc(sizeof(Set));
        if (!set) goto err;
        if (initSet(
                set, sizeof(struct IOTuple),
                (Hash)hashIOTuple, (ValueComparator)compareIOTuples, 
                (FreeValueFunction)freeIOTuple, (PrintValue)printIOTuple
        )) {
            free(set);
            goto err;
        }
        sets[i] = set;
    }
    return sets;
err:
    freeIOSets(sets, i, 1);
    return NULL;
}

void printIOSets(Set **sets, uint64_t num_of_states) {
    for (uint64_t i = 0; i < num_of_states; ++i) {
        printf("%" PRIu64 ": ", i);
        printSet(sets[i]);
        printf("\n");
    }
    printf("\n");
}

struct MemoryCriteriaThreadData {
    Set **sets;
    uint64_t num_of_states;
    uint64_t start_i;
    uint64_t end_i;
    int result;
    atomic_int* global_flag;
};

void* checkMemoryCriteriaThread(void* arg) {
    struct MemoryCriteriaThreadData *data = arg;
    data->result = 1;
    for (uint64_t i = data->start_i; i < data->end_i && atomic_load(data->global_flag) == 1; ++i) {
        for (uint64_t j = i + 1; j < data->num_of_states && atomic_load(data->global_flag) == 1; ++j) {
            Set intersection;
            if (intersectSet(&intersection, data->sets[i], data->sets[j])) {
                atomic_store(data->global_flag, -1);
                data->result = -1;
                return NULL;
            }
            uint8_t intersects = getSetSize(&intersection) != 0;
            freeSet(&intersection, 0);
            if (intersects) {
                atomic_store(data->global_flag, 0);
                data->result = 0;
                return NULL;
            }
        }
    }
    return NULL;
}

int checkMemoryCriteria(Set **sets, uint64_t num_of_states) {
    uint64_t NUM_THREADS = num_of_states - 1 < MAX_NUM_THREADS ? num_of_states - 1 : MAX_NUM_THREADS;
    pthread_t threads[NUM_THREADS];
    struct MemoryCriteriaThreadData thread_data[NUM_THREADS];
    atomic_int global_flag = ATOMIC_VAR_INIT(1);
    uint64_t chunk_size = (num_of_states - 1) / NUM_THREADS;
    uint64_t remaining = (num_of_states - 1) % NUM_THREADS;
    uint64_t start_i = 0;
    for (int t = 0; t < NUM_THREADS; ++t) {
        uint64_t end_i = start_i + chunk_size + (t < remaining ? 1 : 0);
        if (end_i > num_of_states - 1) end_i = num_of_states - 1;
        thread_data[t].sets = sets;
        thread_data[t].num_of_states = num_of_states;
        thread_data[t].start_i = start_i;
        thread_data[t].end_i = end_i;
        thread_data[t].global_flag = &global_flag;
        pthread_create(&threads[t], NULL, checkMemoryCriteriaThread, &thread_data[t]);
        start_i = end_i;
    }
    int final_result = 1;
    for (int t = 0; t < NUM_THREADS; ++t) {
        pthread_join(threads[t], NULL);
        if (thread_data[t].result == -1)  final_result = -1;
        else if (thread_data[t].result == 0 && final_result != -1) final_result = 0;
    }
    return final_result;
}

void printMemory(struct Memory *memory) {
    if (memory->infinite) {
        printf("Память автомата бесконечна.\n");
        return;
    }
    printf("Память автомата равна %" PRIu64 ".\n", memory->memory_size);
    for (uint64_t i = 0; i < 2 * memory->memory_size + 1; ++i) {
        if (memory->memory_table[i]) printIOTuple(memory->memory_table[i]);
        printf("\n");
    }
}

void freeMemory(struct Memory *memory) {
    for (uint64_t i = 0; i < 2 * memory->memory_size + 1; ++i) {
        if (memory->memory_table[i]) {
            freeIOTuple(memory->memory_table[i]);
            free(memory->memory_table[i]);
        }
    }
    free(memory->memory_table);
}

static uint64_t getMemoryTableIndexFromIOTuple(struct IOTuple *io) {
    uint64_t index = 0;
    for (uint64_t i = 0; i < io->current_length; ++i)
        index = (index << 1) | getBitArrayElement(io->input_sequence, i);
    for (uint64_t i = 0; i < io->current_length - 1; ++i)
        index = (index << 1) | getBitArrayElement(io->output_sequence, i);
    return index;
}

int initMemory(
    struct Memory* memory,
    Set **io_sets,
    uint64_t num_of_states,
    uint64_t memory_size,
    uint8_t infinite
) {
    if (infinite) {
        memory->infinite = 1;
        return 0;
    }
    memory->infinite = 0;
    memory->memory_size = memory_size;
    memory->memory_table = malloc(
        ((uint64_t)1 << (2 * memory_size + 1)) * sizeof(struct IOTuple)
    );
    if (!memory->memory_table) return -1;
    for (uint32_t state = 0; state <= (uint32_t)num_of_states - 1; ++state) {
        struct SetIterator it;
        for (
            initSetIterator(io_sets[state], &it);
            !reachedEndSetIterator(&it);
            incSetIterator(&it)
        ) {
            struct IOTuple *io = getSetIteratorValue(&it);
            memory->memory_table[getMemoryTableIndexFromIOTuple(io)] = io;
        }
    }
    return 0;
}