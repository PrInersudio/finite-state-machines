#include <stdexcept>
#include <iostream>
#include <array>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <thread>
#include <atomic>
#include "Redis.hpp"

std::atomic<bool> is_shutting_down(false);

void setShutdownFlag() {
    is_shutting_down.store(true);
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

static std::string join(const std::vector<std::string> &vec, const std::string &sep = " ") {
    std::ostringstream oss;
    for (const std::string &s : vec)
        oss << s << sep;
    return oss.str();
}

std::vector<std::vector<std::string>> RedisContextWrapper::command(const std::vector<std::string> &args) {
    redisReply* reply = nullptr;
    std::vector<std::vector<std::string>> result;

    std::vector<const char*> argv;
    std::vector<size_t> argvlen;
    for (const auto& arg : args) {
        argv.push_back(arg.c_str());
        argvlen.push_back(arg.size());
    }

    for (uint8_t i = 0; i < CONNECTION_ATTEMPTS; ++i) {
        std::lock_guard<std::mutex> lock(this->mutex);
        reply = (redisReply*)redisCommandArgv(this->c, argv.size(), argv.data(), argvlen.data());
        if (reply && reply->type != REDIS_REPLY_ERROR) {
            processRedisReply(reply, result);
            break;
        }
        if (!is_shutting_down.load())
            std::cerr << "Ошибка Redis: " + std::string(this->c->errstr) + 
                ", команда: " + join(args) + ". Попытка подключения " +
                std::to_string(i) + " из " + std::to_string(CONNECTION_ATTEMPTS) + ".\n";
        if (reply) {
            freeReplyObject(reply);
            reply = nullptr;
        }
        std::this_thread::sleep_for(std::chrono::seconds(1 << i));
        this->reconnect();
    }
    if (!reply) {
        std::string error_msg = "Ошибка Redis: " + std::string(this->c->errstr) + 
            ", команда: " + join(args);
        throw std::runtime_error(error_msg);
    }
    freeReplyObject(reply);
    return result;
}

static void exec(const char* cmd) {
    std::array<char, 128> buffer;
    std::string result;
    FILE* pipe = popen(cmd, "r");
    if (!pipe) throw std::runtime_error("Ошибка popen.");
    while (fgets(buffer.data(), buffer.size(), pipe) != nullptr)
        result += buffer.data();
    std::cout << result << std::endl;
    int status = pclose(pipe);
    if (status != 0)
        throw std::runtime_error("Команда завершилась с ошибкой: " + std::to_string(status) + ".");
}

static bool container_debug = false;

void enableContainerDebug() {
    container_debug = true;
}

static std::string getCurrentTimestamp() {
    auto now = std::chrono::system_clock::now();
    std::time_t now_time = std::chrono::system_clock::to_time_t(now);
    std::tm local_tm = *std::localtime(&now_time);
    std::ostringstream oss;
    oss << std::put_time(&local_tm, "%Y-%m-%d_%H-%M-%S");
    return oss.str();
}

static void createDebugDump() {
    std::string prefix = "Memory/redis_dump_" + getCurrentTimestamp();
    std::cout << "[*] Сохраняем состояние Redis в файл dump.rdb..." << std::endl;
    exec("docker compose -f Memory/docker-compose.yaml exec -T redis redis-cli SAVE");
    std::cout << "[*] Копируем дамп Redis с контейнера..." << std::endl;
    exec(("docker cp $(docker compose -f Memory/docker-compose.yaml ps -q redis):/data/dump.rdb " + prefix + ".rdb").c_str());
    std::cout << "[*] Конвертируем дамп в JSON формат..." << std::endl;
    exec(("rdb -c json -o " + prefix + ".json " + prefix + ".rdb").c_str());
    std::cout << "[*] Анализируем JSON дамп..." << std::endl;
    exec(("python3 Memory/analyze_dump.py " + prefix + ".json").c_str());
    std::cout << "[*] Удаляем временные файлы..." << std::endl;
    exec(("rm -f " + prefix + ".json " + prefix + ".rdb").c_str());
}

Container::Container() {
    std::cout << "[*] Удаление старого контейнера Redis (если есть)..." << std::endl;
    exec("docker compose -f Memory/docker-compose.yaml down -v");
    std::cout << "[*] Создание контейнера Redis..." << std::endl;
    exec("docker compose -f Memory/docker-compose.yaml up --build -d");
    std::cout << "[*] Ожидание запуска Redis..." << std::endl;
    try {
        RedisContextWrapper ctx;
        ctx.command({"PING"});
        std::cout << "[+] Redis доступен!" << std::endl;
    } catch (const std::exception& e) {
        throw std::runtime_error("Redis не запустился: " + std::string(e.what()));
    }
}

Container::~Container() {
    if (container_debug)
        try {
            createDebugDump();
        } catch (std::runtime_error &e) {
            std::cerr << "При создании дампа " << e.what() << std::endl;
        }
    std::cout << "[*] Удаление контейнера Redis..." << std::endl;
    exec("docker compose -f Memory/docker-compose.yaml down -v");
    std::cout << "[+] Контейнер удалён." << std::endl;
}
