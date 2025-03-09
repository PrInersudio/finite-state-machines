#include "Hash.h"

uint64_t hashBytes(const void *data, size_t length) {
    uint64_t hash = 5381;
    const uint8_t *ptr = (const uint8_t *)data;
    for (size_t i = 0; i < length; ++i)
        hash = ((hash << 5) + hash) + ptr[i];
    return hash;
}