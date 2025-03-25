#define _GNU_SOURCE
#include "Memory.h"
#include <stdlib.h>
#include <inttypes.h>
#include <pthread.h>
#include <stdatomic.h>
#include <dirent.h>

uint64_t hashIOTuple(struct IOTuple *io) {
    uint64_t hash = hashBitArray(io->input_sequence) * 31 + hashBitArray(io->output_sequence);
    return hash;
}

uint8_t compareIOTuples(struct IOTuple *io1, struct IOTuple *io2) {
    return compareFirstNBytesOfBitArray(io1->input_sequence, io2->input_sequence, (io1->current_length + 7) / 8) &&
        compareFirstNBytesOfBitArray(io1->output_sequence, io2->output_sequence, (io1->current_length + 7) / 8);
}

int putIOTupleIntoFile(struct IOTuple *io, FILE *fp) {
    flockfile(fp);
    uint64_t num_bytes = (io->current_length + 7) / 8;
    int rc = 0;
    if (
        (num_bytes != putNBytesOfBitArrayIntoFile(io->input_sequence, num_bytes, fp)) ||
        (num_bytes != putNBytesOfBitArrayIntoFile(io->output_sequence, num_bytes, fp))
    ) rc = -1;
    funlockfile(fp);
    return rc;
}

int getIOTupleFromFile(struct IOTuple *io, FILE *fp) {
    uint64_t num_bytes = (io->current_length + 7) / 8;
    if (num_bytes != getNBytesOfBitArrayFromFile(io->input_sequence, num_bytes, fp)) return -1;
    if (num_bytes != getNBytesOfBitArrayFromFile(io->output_sequence, num_bytes, fp)) return -2;
    return 0;
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

static FILE* openCacheFile(uint32_t state, uint64_t memory_size, const char *mode) {
    char filename[СACHE_FILE_NAME_LEN];
    snprintf(filename, СACHE_FILE_NAME_LEN, CACHE_FILE_FORMAT "%" PRIu32 "%" PRIu64, state, memory_size);
    FILE *fp = fopen(filename, mode);
    if (!fp) perror("Не удалось открыть файл.");
    return fp;
}

void closeCacheFiles(FILE **files, uint64_t num_of_states) {
    for (uint64_t state = 0; state < num_of_states; ++state)
        fclose(files[state]);
    free(files);
}

FILE **openCacheFiles(uint64_t num_of_states, uint64_t memory_size, const char *mode) {
    FILE **files = malloc(num_of_states * sizeof(FILE *));
    if (!files) return NULL;
    for (uint64_t state = 0; state < num_of_states; ++state) {
        files[state] = openCacheFile(state, memory_size, mode);
        if (!files[state]) {
            closeCacheFiles(files, state);
            return NULL;
        }
    }
    return files;
}

void deleteCacheFiles() {
    DIR *dir = opendir(".");
    if (!dir) {
        perror("Ошибка открытия директории.");
        return;
    }
    struct dirent *entry;
    size_t len_of_cache_file_format = strlen(CACHE_FILE_FORMAT);
    while ((entry = readdir(dir))) {
        if (
            strlen(entry->d_name) < len_of_cache_file_format ||
            strncmp(entry->d_name, CACHE_FILE_FORMAT, len_of_cache_file_format) != 0
        ) continue;
        if (remove(entry->d_name)) perror("Ошибка удаления файла.");
    }
    closedir(dir);
}

int getIOSetFromFile(Set *set, FILE *fp, uint64_t memory_size) {
    int rc = 0;
    if (initSet(
        set, sizeof(struct IOTuple), (Hash)hashIOTuple, (ValueComparator)compareIOTuples,
        (FreeValueFunction)freeIOTuple, (PrintValue)printIOTuple
    )) return -1;
    flockfile(fp);
    while (!feof(fp)) {
        struct IOTuple io;
        if (initIOTuple(&io, memory_size)) {
            rc = -2;
            goto end;
        }
        if (
            getIOTupleFromFile(&io, fp) ||
            pushSet(set, &io)
        ) {
            freeIOTuple(&io);
            rc = -3;
            goto end;
        }
    }
end:
    rewind(fp);
    funlockfile(fp);
    if (rc) freeSet(set, 1);
    return rc;
}

struct MemoryCriteriaThreadData {
    FILE **files;
    uint64_t num_of_states;
    uint64_t memory_size;
    uint32_t start_i;
    uint32_t end_i;
    atomic_int* global_flag;
};

static void* checkMemoryCriteriaThread(void* arg) {
    struct MemoryCriteriaThreadData *data = arg;
    for (uint32_t i = data->start_i; i <= data->end_i - 1 && atomic_load(data->global_flag) == 1; ++i) {
        Set set1;
        if (getIOSetFromFile(&set1, data->files[i], data->memory_size)) return NULL; 
        for (uint32_t j = i + 1; j <= (uint32_t)(data->num_of_states - 1) && atomic_load(data->global_flag) == 1; ++j) {
            Set set2;
            if (getIOSetFromFile(&set2, data->files[j], data->memory_size)) {
                freeSet(&set1, 1);
                atomic_store(data->global_flag, -1);
                return NULL;
            }
            Set intersection;
            if (intersectSet(&intersection, &set1, &set2)) {
                freeSet(&set1, 1);
                freeSet(&set2, 1);
                atomic_store(data->global_flag, -1);
                return NULL;
            }
            uint8_t intersects = getSetSize(&intersection) != 0;
            freeSet(&intersection, 0);
            if (intersects) {
                atomic_store(data->global_flag, 0);
                return NULL;
            }
            freeSet(&set2, 1);
        }
        freeSet(&set1, 1);
    }
    return NULL;
}

int checkMemoryCriteria(uint64_t num_of_states, uint64_t memory_size) {
    FILE **files = openCacheFiles(num_of_states, memory_size, "rb");
    if (!files) return -1;
    uint64_t NUM_THREADS = num_of_states - 1 < MAX_NUM_THREADS ? num_of_states - 1 : MAX_NUM_THREADS;
    pthread_t threads[NUM_THREADS];
    struct MemoryCriteriaThreadData thread_data[NUM_THREADS];
    atomic_int global_flag = ATOMIC_VAR_INIT(1);
    uint32_t chunk_size = (uint32_t)(num_of_states - 1) / NUM_THREADS;
    uint32_t remaining = (uint32_t)(num_of_states - 1) % NUM_THREADS;
    uint32_t start_i = 0;
    for (int t = 0; t < NUM_THREADS; ++t) {
        uint32_t end_i = start_i + chunk_size + (t < remaining ? 1 : 0);
        if (end_i > num_of_states - 1) end_i = num_of_states - 1;
        thread_data[t].files = files;
        thread_data[t].num_of_states = num_of_states;
        thread_data[t].memory_size = memory_size;
        thread_data[t].start_i = start_i;
        thread_data[t].end_i = end_i;
        thread_data[t].global_flag = &global_flag;
        pthread_create(&threads[t], NULL, checkMemoryCriteriaThread, &thread_data[t]);
        start_i = end_i;
    }
    int final_result = 1;
    for (int t = 0; t < NUM_THREADS; ++t) {
        pthread_join(threads[t], NULL);
        if (atomic_load(thread_data[t].global_flag) == -1)  final_result = -1;
        else if (atomic_load(thread_data[t].global_flag) == 0 && final_result != -1) final_result = 0;
    }
    closeCacheFiles(files, num_of_states);
    return final_result;
}