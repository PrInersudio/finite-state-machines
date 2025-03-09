#ifndef HASH_H
#define HASH_H

#include <stdint.h>
#include <string.h>

typedef uint64_t (*Hash)(void *value);

// Простая хэш-функция (djb2).
uint64_t hashBytes(const void *data, size_t length);

#endif