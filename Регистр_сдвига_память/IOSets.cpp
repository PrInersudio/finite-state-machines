#include "IOSets.hpp"
#include <stdexcept>
#include <iostream>

static std::string getIOSetsKey(uint32_t state, uint64_t memory_size) {
    return std::to_string(state) + "_" + std::to_string(memory_size);
}

static std::string getIntersectionKey(uint32_t state1, uint32_t state2, uint64_t memory_size) {
    return std::to_string(state1) + "_" + std::to_string(state2) + "_" + std::to_string(memory_size);
}

RedisContextWrapper::RedisContextWrapper() {
    this->c = redisConnect(IP, PORT);
    if (c == nullptr)
        throw std::runtime_error("Не удалось выделить redis context.");
    if (c->err)
        throw std::runtime_error("Ошибка при подключении к базе данных: " + std::string(c->errstr) + ".");
}

RedisContextWrapper::~RedisContextWrapper() {
    redisFree(this->c);
}

void RedisContextWrapper::reconnect() {
    redisFree(this->c);
    this->c = redisConnect(IP, PORT);
    if (c == nullptr)
        throw std::runtime_error("Не удалось выделить redis context.");
    if (c->err)
        throw std::runtime_error("Ошибка при подключении к базе данных: " + std::string(c->errstr) + ".");
}

static void processRedisReply(redisReply *reply, std::vector<std::vector<std::string>>& output) {
    switch (reply->type) {
        case REDIS_REPLY_ARRAY:
            output.resize(reply->elements);
            for (size_t i = 0; i < reply->elements; ++i)
                if (reply->element[i]->type == REDIS_REPLY_ARRAY) {
                    for (size_t j = 0; j < reply->element[i]->elements; ++j)
                        if (reply->element[i]->element[j]->str)
                            output[i].emplace_back(reply->element[i]->element[j]->str);
                } else if (reply->element[i]->str)
                    output[i].emplace_back(reply->element[i]->str);
            break;
        case REDIS_REPLY_STRING:
            output.resize(1);
            output[0].emplace_back(reply->str);
            break;
        case REDIS_REPLY_INTEGER:
            output.resize(1);
            output[0].emplace_back(std::to_string(reply->integer));
            break;
        default:
            break;
    }
}

std::vector<std::vector<std::string>> RedisContextWrapper::command(const char* format, ...) {
    redisReply* reply = nullptr;
    std::vector<std::vector<std::string>> result;
    va_list ap;
    va_start(ap, format);
    for (uint8_t i = 0; i < CONNECTION_ATTEMPTS; ++i) {
        va_list ap_copy;
        va_copy(ap_copy, ap);
        reply = (redisReply*)redisvCommand(this->c, format, ap_copy);
        va_end(ap_copy);
        if (reply && reply->type != REDIS_REPLY_ERROR) {
            processRedisReply(reply, result);
            break;
        }
        std::cerr << "Ошибка Redis: " + std::string(this->c->errstr) + 
            ", команда: " + std::string(format) + ". Попытка подключения " +
            std::to_string(i) + " из " + std::to_string(CONNECTION_ATTEMPTS) + ".\n";
        if (reply) {
            freeReplyObject(reply);
            reply = nullptr;
        }
        this->reconnect();
    }
    if (!reply) {
        std::string error_msg = "Ошибка Redis: " + std::string(this->c->errstr) + 
            ", команда: " + std::string(format);
        throw std::runtime_error(error_msg);
    }
    freeReplyObject(reply);
    return result;
}

IOSets::IOSets(uint64_t memory_size) : memory_size(memory_size) {}

void IOSets::clear(RedisContextWrapper &ctx) {
    for (uint32_t state : this->actual_states)
        ctx.command("DEL %s", getIOSetsKey(state, this->memory_size).c_str());
    this->actual_states.clear();
}

void IOSets::insert(RedisContextWrapper &ctx, uint32_t state, const IOTuple &io) {
    ctx.command("SADD %s %s", getIOSetsKey(state, this->memory_size).c_str(), io.to_string().c_str());
    this->actual_states.insert(state);
}

IOSets::Iterator::Iterator(RedisContextWrapper &ctx, const std::string& k, bool end)
    : ctx(ctx), key(k), cursor("0"), is_end(end) {
    if (!is_end) this->fetch_next_batch();
}

void IOSets::Iterator::fetch_next_batch() {
    std::vector<std::vector<std::string>> reply = ctx.command("SSCAN %s %s", key.c_str(), cursor.c_str());
    this->cursor = reply[0][0];
    this->current_batch.clear();
    this->current_batch.reserve(reply[1].size());
    for (const std::string &item_str : reply[1])
    this->current_batch.emplace_back(item_str);
    this->batch_pos = 0;
    this->is_end = (cursor == "0") && this->current_batch.empty();
}

IOSets::Iterator& IOSets::Iterator::operator++() {
    if (++this->batch_pos >= this->current_batch.size()) {
        if (this->cursor != "0") this->fetch_next_batch();
        else this->is_end = 1;
    }
    return *this;
}

IOTuple IOSets::Iterator::operator*() const {
    return this->current_batch[this->batch_pos];
}

bool IOSets::Iterator::operator!=(const Iterator& other) const {
    return this->is_end != other.is_end;
}

IOSets::Iterator IOSets::begin(RedisContextWrapper &ctx, uint32_t state) {
    return Iterator(ctx, getIOSetsKey(state, this->memory_size));
}

IOSets::Iterator IOSets::end(RedisContextWrapper &ctx, uint32_t state) {
    return Iterator(ctx, getIOSetsKey(state, this->memory_size), true);
}

std::unordered_set<uint32_t> &IOSets::getActualStates() {
    return this->actual_states;
}

uint64_t IOSets::getMemorySize() const{
    return memory_size;
}

bool IOSets::intersects(RedisContextWrapper &ctx, uint32_t state1, uint32_t state2) const {
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

bool IOSets::empty() const {
    return this->actual_states.empty();
}