#include <stdexcept>
#include <iostream>
#include "RedisContextWrapper.hpp"

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