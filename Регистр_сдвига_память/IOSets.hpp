#ifndef IOSETS_HPP
#define IOSETS_HPP

#include<hiredis/hiredis.h>
#include <unordered_set>
#include "IOTuple.hpp"

#define IP "127.0.0.1"
#define PORT 6379
#define CONNECTION_ATTEMPTS 11

class RedisContextWrapper {
private:
    redisContext* c;

    void reconnect();
public:
    RedisContextWrapper();
    std::vector<std::vector<std::string>> command(const char* format, ...);
    ~RedisContextWrapper();
};

class IOSets {
private:
    std::unordered_set<uint32_t> actual_states;
    uint64_t memory_size;
public:
    IOSets(uint64_t memory_size);
    void insert(RedisContextWrapper &ctx, uint32_t state, const IOTuple &io);
    std::unordered_set<uint32_t> &getActualStates();
    uint64_t getMemorySize() const;
    class Iterator;
    Iterator begin(RedisContextWrapper &ctx, uint32_t state);
    Iterator end(RedisContextWrapper &ctx, uint32_t state);
    bool intersects(RedisContextWrapper &ctx, uint32_t state1, uint32_t state2) const;
    bool empty() const;
    void clear(RedisContextWrapper &ctx);
};

class IOSets::Iterator {
private:
    RedisContextWrapper &ctx;
    std::string key;
    std::string cursor;
    bool is_end;
    std::vector<IOTuple> current_batch;
    size_t batch_pos = 0;

    void fetch_next_batch();
public:
    Iterator(RedisContextWrapper &ctx, const std::string& k, bool end = false);
    Iterator& operator++();
    IOTuple operator*() const;
    bool operator!=(const Iterator &other) const;
};

#endif