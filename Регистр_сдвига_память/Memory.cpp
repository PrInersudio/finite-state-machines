#include "Memory.hpp"
#include <set>
#include <tuple>
#include <iostream>
#include <thread>
#include <atomic>
#include <mutex>
#include <future>
#include <algorithm>

class IOTuple {
private:
    std::vector<bool> input;
    std::vector<bool> output;
public:
    IOTuple();
    IOTuple(const IOTuple& other);
    void push(bool input, bool output);
    bool operator<(const IOTuple &other) const;
    friend std::ostream &operator<<(std::ostream &os, const IOTuple &io);
};

IOTuple::IOTuple() {}
IOTuple::IOTuple(const IOTuple& other) : input(other.input), output(other.output) {}

void IOTuple::push(bool input, bool output) {
    this->input.push_back(input);
    this->output.push_back(output);
}

bool IOTuple::operator<(const IOTuple &other) const {
    return std::tie(this->input, this->output) < std::tie(other.input, other.output);
}

std::ostream &operator<<(std::ostream &os, const IOTuple &io) {
    os << "(";
    for (bool x : io.input) os << x;
    os << ", ";
    for (bool y : io.output) os << y;
    os << ")";
    return os;
}

static std::ostream &operator<<(std::ostream &os, const std::vector<std::set<IOTuple>> &sets) {
    for (uint32_t state = 0; state <= static_cast<uint32_t>(sets.size() - 1); ++state) {
        os << state << ": { ";
        for (const IOTuple &io : sets[state]) os << io << " ";
        os << "} " << std::endl;
    }
    return os;
}

static std::vector<std::set<IOTuple>> initIOSets(ShiftRegister &reg, uint64_t upper_bound) {
    std::vector<std::set<IOTuple>> sets(static_cast<uint64_t>(1) << reg.getLength());
    for (uint32_t state = 0; state <= (static_cast<uint32_t>(1) << reg.getLength()) - 1; ++state)
        for (bool x : {false, true}) {
            IOTuple io;
            io.push(x, reg.outputFunction(state, x));
            sets[reg.stateFunction(state, x)].insert(io);
        }
    return sets;
}

static void updateIOSetsThread(
    ShiftRegister &reg,
    const std::vector<std::set<IOTuple>> &sets,
    std::vector<std::set<IOTuple>> &new_sets,
    uint32_t start_state,
    uint32_t end_state,
    std::vector<std::mutex> &mutexes 
) {
    for (uint32_t state = start_state; state < end_state; ++state)
        for(const IOTuple &io : sets[state])
            for (bool x : {false, true}) {
                IOTuple new_io = io;
                new_io.push(x, reg.outputFunction(state, x));
                uint32_t next_state = reg.stateFunction(state, x);
                {
                    std::lock_guard<std::mutex> lock(mutexes[next_state % mutexes.size()]);
                    new_sets[next_state].insert(new_io);
                }
            }
}

static void updateIOSets(std::vector<std::set<IOTuple>> &sets, ShiftRegister &reg) {
    uint64_t num_of_states = static_cast<uint64_t>(1) << reg.getLength();
    uint NUM_THREADS = std::min(static_cast<uint>(num_of_states), std::thread::hardware_concurrency());
    std::vector<std::set<IOTuple>> new_sets(num_of_states);
    std::vector<std::mutex> mutexes(std::min(static_cast<uint>(num_of_states), 256u));
    std::vector<std::future<void>> futures;
    uint32_t chunk_size = num_of_states / NUM_THREADS;
    uint32_t remaining = num_of_states % NUM_THREADS;
    uint32_t start_state = 0;
    for (uint t = 0; t < NUM_THREADS; ++t) {
        uint32_t end_state = start_state + chunk_size + (t < remaining ? 1 : 0);
        futures.push_back(std::async(
            std::launch::async, updateIOSetsThread, std::ref(reg), std::cref(sets),
            std::ref(new_sets), start_state, end_state, std::ref(mutexes)
        ));
        start_state = end_state;
    }
    for (auto &t : futures) t.get();
    sets = std::move(new_sets);
}

static void checkMemoryCriteriaThread(
    const std::vector<std::set<IOTuple>> &sets,
    uint32_t start_state,
    uint32_t end_state,
    std::atomic<bool> &result
) {
    for (uint32_t i = start_state; i < end_state && result.load(); ++i)
        for (uint32_t j = i + 1; j <= static_cast<uint32_t>(sets.size() - 1) && result.load(); ++j) {
            std::set<IOTuple> intersection;
            std::set_intersection(
                sets[i].begin(), sets[i].end(),
                sets[j].begin(), sets[j].end(),
                std::inserter(intersection, intersection.begin())
            );
            if (intersection.size() != 0) {
                result.store(false);
                return;
            }
        }
}

static bool checkMemoryCriteria(const std::vector<std::set<IOTuple>> &sets) {
    uint NUM_THREADS = std::min(static_cast<uint>(sets.size()), std::thread::hardware_concurrency());
    std::atomic<bool> result(true);
    std::vector<std::future<void>> futures;
    uint32_t chunk_size = (sets.size() - 1) / NUM_THREADS;
    uint32_t remaining = (sets.size() - 1) % NUM_THREADS;
    uint64_t start_state = 0;
    for (uint t = 0; t < NUM_THREADS; ++t) {
        uint32_t end_state = start_state + chunk_size + (t < remaining ? 1 : 0);
        if (end_state > sets.size() - 1) end_state = sets.size() - 1;
        futures.push_back(std::async(
            std::launch::async, checkMemoryCriteriaThread, std::cref(sets),
            start_state, end_state, std::ref(result)
        ));
        start_state = end_state;
    }
    for (auto &t : futures) t.get();
    return result.load();
}

static uint64_t countMemory(
    ShiftRegister reg,
    uint64_t upper_bound
) {
    uint64_t memory_size;
    std::vector<std::set<IOTuple>> sets = initIOSets(reg, upper_bound);
    for (memory_size = 1; memory_size <= upper_bound; ++memory_size) {
        std::cerr << "countMemory memory_size = " << memory_size << std::endl << sets;
        if (checkMemoryCriteria(sets)) break;
        updateIOSets(sets, reg);
    }
    return memory_size;
}

void getMemoryShiftRegister(ShiftRegister &reg, uint64_t upper_bound) {
    uint64_t memory_size = countMemory(reg, upper_bound);
    if (memory_size > upper_bound)
        std::cout << "Память автомата бесконечна." << std::endl;
    else
        std::cout << "Память автомата равна" << memory_size << std::endl;
}
