#ifndef IOSETS_HPP
#define IOSETS_HPP

#include <unordered_set>
#include <sstream>
#include "Redis.hpp"

template <typename StateType>
static std::string stateToString(const StateType &state) {
    std::ostringstream oss;
    oss << state;
    return oss.str();
}

template <typename IOType, typename StateType>
class IOSets {
private:
    std::unordered_set<StateType> actual_states;
    uint64_t memory_size;
public:
    IOSets(uint64_t memory_size);
    void insert(RedisContextWrapper &ctx, const StateType &state, const IOType &io);
    const std::unordered_set<StateType> &getActualStates() const;
    uint64_t getMemorySize() const;
    class Iterator;
    Iterator begin(RedisContextWrapper &ctx, const StateType &state) const;
    Iterator end(RedisContextWrapper &ctx, const StateType &state) const;
    bool intersects(RedisContextWrapper &ctx, const StateType &state1, const StateType &state2) const;
    bool empty() const;
    void clear(RedisContextWrapper &ctx);
};

template <typename IOType, typename StateType>
class IOSets<IOType, StateType>::Iterator {
private:
    RedisContextWrapper &ctx;
    const std::string key;
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

template <typename StateType>
static std::string getIOSetsKey(const StateType &state, uint64_t memory_size) {
    return stateToString(state) + "_" + std::to_string(memory_size);
}

template <typename StateType>
static std::string getIntersectionKey(const StateType &state1, const StateType &state2, uint64_t memory_size) {
    return stateToString(state1) + "_" + stateToString(state2) + "_" + std::to_string(memory_size);
}

template <typename IOType, typename StateType>
IOSets<IOType, StateType>::IOSets(uint64_t memory_size) : memory_size(memory_size) {}

template <typename IOType, typename StateType>
void IOSets<IOType, StateType>::clear(RedisContextWrapper &ctx) {
    for (const StateType &state : this->actual_states)
        ctx.command({"DEL", getIOSetsKey(state, this->memory_size)});
    this->actual_states.clear();
}

template <typename IOType, typename StateType>
void IOSets<IOType, StateType>::insert(RedisContextWrapper &ctx, const StateType &state, const IOType &io) {
    ctx.command({"SADD", getIOSetsKey(state, this->memory_size), io.toString()});
    this->actual_states.insert(state);
}

template <typename IOType, typename StateType>
IOSets<IOType, StateType>::Iterator::Iterator(RedisContextWrapper &ctx, const std::string& k, bool end)
    : ctx(ctx), key(k), cursor("0"), is_end(end) {
    if (!is_end) this->fetch_next_batch();
}

template <typename IOType, typename StateType>
void IOSets<IOType, StateType>::Iterator::fetch_next_batch() {
    std::vector<std::vector<std::string>> reply = ctx.command({"SSCAN", key, cursor});
    this->cursor = reply[0][0];
    this->current_batch.clear();
    this->current_batch.reserve(reply[1].size());
    for (const std::string &item_str : reply[1])
    this->current_batch.emplace_back(item_str);
    this->batch_pos = 0;
    this->is_end = (cursor == "0") && this->current_batch.empty();
}

template <typename IOType, typename StateType>
typename IOSets<IOType, StateType>::Iterator &IOSets<IOType, StateType>::Iterator::operator++() {
    if (++this->batch_pos >= this->current_batch.size()) {
        if (this->cursor != "0") this->fetch_next_batch();
        else this->is_end = 1;
    }
    return *this;
}

template <typename IOType, typename StateType>
IOType IOSets<IOType, StateType>::Iterator::operator*() const {
    return this->current_batch[this->batch_pos];
}

template <typename IOType, typename StateType>
bool IOSets<IOType, StateType>::Iterator::operator!=(const Iterator& other) const {
    return this->is_end != other.is_end;
}

template <typename IOType, typename StateType>
typename IOSets<IOType, StateType>::Iterator IOSets<IOType, StateType>::begin(
    RedisContextWrapper &ctx,
    const StateType &state
) const {
    return Iterator(ctx, getIOSetsKey(state, this->memory_size));
}

template <typename IOType, typename StateType>
typename IOSets<IOType, StateType>::Iterator IOSets<IOType, StateType>::end(
    RedisContextWrapper &ctx,
    const StateType &state
) const {
    return Iterator(ctx, getIOSetsKey(state, this->memory_size), true);
}

template <typename IOType, typename StateType>
const std::unordered_set<StateType> &IOSets<IOType, StateType>::getActualStates() const {
    return this->actual_states;
}

template <typename IOType, typename StateType>
uint64_t IOSets<IOType, StateType>::getMemorySize() const{
    return memory_size;
}

template <typename IOType, typename StateType>
bool IOSets<IOType, StateType>::intersects(
    RedisContextWrapper &ctx,
    const StateType &state1,
    const StateType &state2
) const {
    std::string intersection_key = getIntersectionKey(state1, state2, this->memory_size);
    ctx.command({"SINTERSTORE", 
        intersection_key, getIOSetsKey(state1, memory_size), getIOSetsKey(state2, memory_size)});
    auto exists_reply = ctx.command({"EXISTS", intersection_key});
    bool intersects = std::stoi(exists_reply[0][0]) > 0;
    ctx.command({"DEL", intersection_key});
    return intersects;
}

template <typename IOType, typename StateType>
bool IOSets<IOType, StateType>::empty() const {
    return this->actual_states.empty();
}

#endif
