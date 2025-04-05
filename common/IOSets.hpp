#ifndef IOSETS_HPP
#define IOSETS_HPP

#include <unordered_set>
#include "RedisContextWrapper.hpp"

template <typename IOType>
class IOSets {
private:
    std::unordered_set<uint32_t> actual_states;
    uint64_t memory_size;
public:
    IOSets(uint64_t memory_size);
    void insert(RedisContextWrapper &ctx, uint32_t state, const IOType &io);
    std::unordered_set<uint32_t> &getActualStates();
    uint64_t getMemorySize() const;
    class Iterator;
    Iterator begin(RedisContextWrapper &ctx, uint32_t state);
    Iterator end(RedisContextWrapper &ctx, uint32_t state);
    bool intersects(RedisContextWrapper &ctx, uint32_t state1, uint32_t state2) const;
    bool empty() const;
    void clear(RedisContextWrapper &ctx);
};

template <typename IOType>
class IOSets<IOType>::Iterator {
private:
    RedisContextWrapper &ctx;
    std::string key;
    std::string cursor;
    bool is_end;
    std::vector<IOType> current_batch;
    size_t batch_pos = 0;

    void fetch_next_batch();
public:
    Iterator(RedisContextWrapper &ctx, const std::string& k, bool end = false);
    Iterator& operator++();
    IOType operator*() const;
    bool operator!=(const Iterator &other) const;
};

static std::string getIOSetsKey(uint32_t state, uint64_t memory_size) {
    return std::to_string(state) + "_" + std::to_string(memory_size);
}

static std::string getIntersectionKey(uint32_t state1, uint32_t state2, uint64_t memory_size) {
    return std::to_string(state1) + "_" + std::to_string(state2) + "_" + std::to_string(memory_size);
}

template <typename IOType>
IOSets<IOType>::IOSets(uint64_t memory_size) : memory_size(memory_size) {}

template <typename IOType>
void IOSets<IOType>::clear(RedisContextWrapper &ctx) {
    for (uint32_t state : this->actual_states)
        ctx.command("DEL %s", getIOSetsKey(state, this->memory_size).c_str());
    this->actual_states.clear();
}

template <typename IOType>
void IOSets<IOType>::insert(RedisContextWrapper &ctx, uint32_t state, const IOType &io) {
    ctx.command("SADD %s %s", getIOSetsKey(state, this->memory_size).c_str(), io.to_string().c_str());
    this->actual_states.insert(state);
}

template <typename IOType>
IOSets<IOType>::Iterator::Iterator(RedisContextWrapper &ctx, const std::string& k, bool end)
    : ctx(ctx), key(k), cursor("0"), is_end(end) {
    if (!is_end) this->fetch_next_batch();
}

template <typename IOType>
void IOSets<IOType>::Iterator::fetch_next_batch() {
    std::vector<std::vector<std::string>> reply = ctx.command("SSCAN %s %s", key.c_str(), cursor.c_str());
    this->cursor = reply[0][0];
    this->current_batch.clear();
    this->current_batch.reserve(reply[1].size());
    for (const std::string &item_str : reply[1])
    this->current_batch.emplace_back(item_str);
    this->batch_pos = 0;
    this->is_end = (cursor == "0") && this->current_batch.empty();
}

template <typename IOType>
typename IOSets<IOType>::Iterator& IOSets<IOType>::Iterator::operator++() {
    if (++this->batch_pos >= this->current_batch.size()) {
        if (this->cursor != "0") this->fetch_next_batch();
        else this->is_end = 1;
    }
    return *this;
}

template <typename IOType>
IOType IOSets<IOType>::Iterator::operator*() const {
    return this->current_batch[this->batch_pos];
}

template <typename IOType>
bool IOSets<IOType>::Iterator::operator!=(const Iterator& other) const {
    return this->is_end != other.is_end;
}

template <typename IOType>
typename IOSets<IOType>::Iterator IOSets<IOType>::begin(RedisContextWrapper &ctx, uint32_t state) {
    return Iterator(ctx, getIOSetsKey(state, this->memory_size));
}

template <typename IOType>
typename IOSets<IOType>::Iterator IOSets<IOType>::end(RedisContextWrapper &ctx, uint32_t state) {
    return Iterator(ctx, getIOSetsKey(state, this->memory_size), true);
}

template <typename IOType>
std::unordered_set<uint32_t> &IOSets<IOType>::getActualStates() {
    return this->actual_states;
}

template <typename IOType>
uint64_t IOSets<IOType>::getMemorySize() const{
    return memory_size;
}

template <typename IOType>
bool IOSets<IOType>::intersects(RedisContextWrapper &ctx, uint32_t state1, uint32_t state2) const {
    std::string intersection_key = getIntersectionKey(state1, state2, this->memory_size);
    ctx.command("SINTERSTORE %s %s %s", 
        intersection_key.c_str(), 
        getIOSetsKey(state1, memory_size).c_str(), 
        getIOSetsKey(state2, memory_size).c_str());
    auto exists_reply = ctx.command("EXISTS %s", intersection_key.c_str());
    bool intersects = std::stoi(exists_reply[0][0]) > 0;
    ctx.command("DEL %s", intersection_key.c_str());
    return intersects;
}

template <typename IOType>
bool IOSets<IOType>::empty() const {
    return this->actual_states.empty();
}

#endif