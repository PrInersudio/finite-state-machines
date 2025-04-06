#ifndef IOSETS_HPP
#define IOSETS_HPP

#include <unordered_set>
#include <sqlite3.h>
#include <filesystem>

#define DB_FILE "IOSets.db"

template <typename StateType>
static std::string stateToString(const StateType &state) {
    std::ostringstream oss;
    oss << state;
    return oss.str();
}


class Sqlite3ConnectionWrapper {
private:
    sqlite3 *db;
public:
    Sqlite3ConnectionWrapper();
    ~Sqlite3ConnectionWrapper();
    sqlite3 *get() const;
};

Sqlite3ConnectionWrapper::~Sqlite3ConnectionWrapper() {
    if (this->db) sqlite3_close(this->db);
#ifndef DEBUG
    std::filesystem::remove(DB_FILE);
#endif
}

Sqlite3ConnectionWrapper::Sqlite3ConnectionWrapper() {
    if (sqlite3_open(DB_FILE, &this->db) != SQLITE_OK)
        throw std::runtime_error("Не удалось создать базу данных для множеств.");
    char *errmsg;
    if (sqlite3_exec(this->db, 
        "CREATE TABLE IF NOT EXISTS iosets ("
        "memory_size INTEGER,"
        "state TEXT,"
        "value TEXT,"
        "PRIMARY KEY (memory_size, state, value));",
        nullptr, nullptr, &errmsg) != SQLITE_OK) {

        std::string err = errmsg;
        sqlite3_free(errmsg);
        throw std::runtime_error("Ошибка создания таблицы: " + err);
    }
}

sqlite3 *Sqlite3ConnectionWrapper::get() const {
    return this->db;
}

Sqlite3ConnectionWrapper connection;

template <typename IOType, typename StateType>
class IOSets {
private:
    std::unordered_set<StateType> actual_states;
    uint64_t memory_size;
public:
    IOSets(uint64_t memory_size);
    void insert(const StateType &state, const IOType &io);
    std::unordered_set<StateType> &getActualStates();
    uint64_t getMemorySize() const;
    class Iterator;
    Iterator begin(const StateType &state) const;
    Iterator end(const StateType &state) const;
    bool intersects(const StateType &state1, const StateType &state2) const;
    bool empty() const;
    void clear();
};

template <typename IOType, typename StateType>
class IOSets<IOType, StateType>::Iterator {
private:
    sqlite3_stmt* stmt;
    bool is_end;
public:
    Iterator(const uint64_t memory_size, const StateType &state, const bool end);
    ~Iterator();
    Iterator &operator++();
    IOType operator*() const;
    bool operator!=(const Iterator &other) const;
};

template <typename IOType, typename StateType>
IOSets<IOType, StateType>::IOSets(uint64_t memory_size) : memory_size(memory_size) {}

template <typename IOType, typename StateType>
void IOSets<IOType, StateType>::clear() {
#ifndef DEBUG
    sqlite3_stmt* stmt;
    if (
        sqlite3_prepare_v2(connection.get(),
            "DELETE FROM iosets WHERE memory_size = ?;",
            -1, &stmt, nullptr) != SQLITE_OK ||
        sqlite3_bind_int64(stmt, 1, memory_size) != SQLITE_OK ||
        sqlite3_step(stmt) != SQLITE_DONE
    ) {} // В целом не важно. Не удалилось, значит не удалилось.
    if (stmt) sqlite3_finalize(stmt);
#endif
    actual_states.clear();
}

template <typename IOType, typename StateType>
void IOSets<IOType, StateType>::insert(const StateType &state, const IOType &io) {
    sqlite3_stmt* stmt;
    if (
        sqlite3_prepare_v2(connection.get(), 
            "INSERT OR IGNORE INTO iosets (memory_size, state, value) VALUES (?, ?, ?);",
            -1, &stmt, nullptr) != SQLITE_OK ||
        sqlite3_bind_int64(stmt, 1, memory_size) != SQLITE_OK ||
        sqlite3_bind_text(stmt, 2, stateToString(state).c_str(), -1, SQLITE_TRANSIENT) != SQLITE_OK ||
        sqlite3_bind_text(stmt, 3, io.toString().c_str(), -1, SQLITE_TRANSIENT) != SQLITE_OK
    ) {
        if (stmt) sqlite3_finalize(stmt);
        throw std::runtime_error("Ошибка подготовки запроса insert. Память: " +
            std::to_string(this->memory_size) +
            ", состояние: " + stateToString(state) +
            ", io: " + io.toString() + ".");
    }
    if (sqlite3_step(stmt) != SQLITE_DONE) {
        sqlite3_finalize(stmt);
        throw std::runtime_error("Ошибка выполнения запроса insert. Память: " +
            std::to_string(this->memory_size) +
            ", состояние: " + stateToString(state) +
            ", io: " + io.toString() + ".");
    }
    sqlite3_finalize(stmt);
    actual_states.insert(state);
}

template <typename IOType, typename StateType>
IOSets<IOType, StateType>::Iterator::Iterator(const uint64_t memory_size, 
    const StateType &state, const bool end) : stmt(nullptr), is_end(end) {
    
    if (this->is_end) return;
    if (
        sqlite3_prepare_v2(connection.get(),
            "SELECT value FROM iosets WHERE memory_size = ? AND state = ?;",
            -1, &stmt, nullptr) != SQLITE_OK ||
        sqlite3_bind_int64(stmt, 1, memory_size) != SQLITE_OK ||
        sqlite3_bind_text(stmt, 2, stateToString(state).c_str(), -1, SQLITE_TRANSIENT) != SQLITE_OK
    ) {
        if (stmt) sqlite3_finalize(stmt);
        throw std::runtime_error("Ошибка подготовки запроса select. Память: " +
            std::to_string(memory_size) +
            ", состояние: " + stateToString(state) + ".");
    }
    this->is_end = (sqlite3_step(stmt) != SQLITE_ROW);
}

template <typename IOType, typename StateType>
IOSets<IOType, StateType>::Iterator::~Iterator() {
    if (stmt) sqlite3_finalize(stmt);
}

template <typename IOType, typename StateType>
typename IOSets<IOType, StateType>::Iterator &IOSets<IOType, StateType>::Iterator::operator++() {
    if (sqlite3_step(stmt) != SQLITE_ROW)
        this->is_end = true;
    return *this;
}

template <typename IOType, typename StateType>
IOType IOSets<IOType, StateType>::Iterator::operator*() const {
    return IOType(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0)));
}

template <typename IOType, typename StateType>
bool IOSets<IOType, StateType>::Iterator::operator!=(const Iterator& other) const {
    return this->is_end != other.is_end;
}

template <typename IOType, typename StateType>
typename IOSets<IOType, StateType>::Iterator IOSets<IOType, StateType>::begin(const StateType &state) const {
    return Iterator(this->memory_size, state, false);
}

template <typename IOType, typename StateType>
typename IOSets<IOType, StateType>::Iterator IOSets<IOType, StateType>::end(const StateType &state) const {
    return Iterator(this->memory_size, state, true);
}

template <typename IOType, typename StateType>
std::unordered_set<StateType> &IOSets<IOType, StateType>::getActualStates() {
    return this->actual_states;
}

template <typename IOType, typename StateType>
uint64_t IOSets<IOType, StateType>::getMemorySize() const{
    return memory_size;
}

template <typename IOType, typename StateType>
bool IOSets<IOType, StateType>::intersects(const StateType &state1, const StateType &state2) const {
    sqlite3_stmt* stmt;
    if (
        sqlite3_prepare_v2(connection.get(),
            "SELECT 1 FROM iosets i1 "
            "INNER JOIN iosets i2 ON i1.value = i2.value "
            "WHERE i1.memory_size = ? AND i2.memory_size = ? "
            "AND i1.state = ? AND i2.state = ? LIMIT 1;",
            -1, &stmt, nullptr) != SQLITE_OK ||
            sqlite3_bind_int64(stmt, 1, this->memory_size) != SQLITE_OK ||
            sqlite3_bind_int64(stmt, 2, this->memory_size) != SQLITE_OK ||
            sqlite3_bind_text(stmt, 3, stateToString(state1).c_str(), -1, SQLITE_TRANSIENT) != SQLITE_OK ||
            sqlite3_bind_text(stmt, 4, stateToString(state2).c_str(), -1, SQLITE_TRANSIENT) != SQLITE_OK
    ) {
        if (stmt) sqlite3_finalize(stmt);
        throw std::runtime_error("Ошибка подготовки запроса intersects. Память: " +
            std::to_string(this->memory_size) +
            ", состояние 1: " + std::to_string(state1) +
            ", состояние 2: " + std::to_string(state2) + ".");
    }
    bool result = (sqlite3_step(stmt) == SQLITE_ROW);
    sqlite3_finalize(stmt);
    return result;
}

template <typename IOType, typename StateType>
bool IOSets<IOType, StateType>::empty() const {
    return this->actual_states.empty();
}

#endif