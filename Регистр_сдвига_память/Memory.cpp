#include "Memory.hpp"
#include <set>
#include <tuple>
#include <iostream>
#include <thread>
#include <atomic>
#include <mutex>
#include <future>
#include <algorithm>
#include <unordered_map>

class IOTuple {
private:
    std::vector<bool> input;
    std::vector<bool> output;
public:
    IOTuple(bool input, bool output);
    IOTuple(const IOTuple& other);
    void push(bool input, bool output);
    bool operator<(const IOTuple &other) const;
    bool operator==(const IOTuple& other) const;
    friend std::ostream &operator<<(std::ostream &os, const IOTuple &io);
};

IOTuple::IOTuple(bool input, bool output) {
    this->push(input, output);
}
IOTuple::IOTuple(const IOTuple& other) : input(other.input), output(other.output) {}

void IOTuple::push(bool input, bool output) {
    this->input.push_back(input);
    this->output.push_back(output);
}

bool IOTuple::operator<(const IOTuple &other) const {
    return std::tie(this->input, this->output) < std::tie(other.input, other.output);
}

bool IOTuple::operator==(const IOTuple& other) const {
    return this->input == other.input && this->output == other.output;
}

std::ostream &operator<<(std::ostream &os, const IOTuple &io) {
    os << "(";
    for (bool x : io.input) os << x;
    os << ", ";
    for (bool y : io.output) os << y;
    os << ")";
    return os;
}

using IOSets = std::unordered_map<uint32_t, std::set<IOTuple>>;

static std::ostream &operator<<(std::ostream &os, const IOSets &sets) {
    for (const auto &[state, set] : sets) {
        os << state << ": { ";
        for (const IOTuple &io : set) os << io << " ";
        os << "} " << std::endl;
    }
    return os;
}

static IOSets initIOSets(ShiftRegister &reg, uint64_t upper_bound) {
    IOSets sets;
    for (uint32_t state = 0; state <= (static_cast<uint32_t>(1) << reg.getLength()) - 1; ++state)
        for (bool x : {false, true})
            sets[reg.stateFunction(state, x)].emplace(x, reg.outputFunction(state, x));
    return sets;
}


static void updateIOSets(IOSets&sets, ShiftRegister &reg) {
    IOSets new_sets;
    std::vector<uint32_t> active_states;
    active_states.reserve(sets.size());
    for (const auto &[state, _] : sets)
        active_states.push_back(state);

    auto worker = [&](uint32_t start, uint32_t end) {
        IOSets local_sets;
        for (uint32_t i = start; i < end; ++i) {
            const uint32_t &state = active_states[i];
            for (const IOTuple &io : sets[state])
                for (bool x : {false, true}) {
                    IOTuple new_io = io;
                    new_io.push(x, reg.outputFunction(state, x));
                    local_sets[reg.stateFunction(state, x)].insert(std::move(new_io));
                }
        }
        return local_sets;
    };

    const unsigned num_threads = std::min<unsigned>(
        std::thread::hardware_concurrency(),
        active_states.size()
    );
    std::vector<std::future<IOSets>> futures;
    futures.reserve(num_threads);
    const uint32_t base_chunk = active_states.size() / num_threads;
    const uint32_t remainder = active_states.size() % num_threads;
    uint32_t start = 0;
    for (unsigned i = 0; i < num_threads; ++i) {
        uint32_t end = start + base_chunk + (i < remainder ? 1 : 0);
        futures.emplace_back(std::async(std::launch::async, worker, start, end));
        start = end;
    }
    for (auto &future : futures)
        for (const auto &[state, set] : future.get())
            new_sets[state].insert(set.begin(), set.end());
    sets = std::move(new_sets);
}

static bool checkMemoryCriteria(const IOSets &sets) {
    if (sets.empty()) return true;
    std::atomic<bool> result(true);
    std::vector<uint32_t> active_states;
    active_states.reserve(sets.size());
    for (const auto& [state, _] : sets)
        active_states.push_back(state);

    auto worker = [&](uint32_t start, uint32_t end) {
        for (uint32_t i = start; i < end && result.load(std::memory_order_relaxed); ++i) {
            const auto &set1 = sets.at(active_states[i]);
            for (uint32_t j = i + 1; j < active_states.size() && result.load(std::memory_order_relaxed); ++j) {
                const auto &set2 = sets.at(active_states[j]);
                auto it1 = set1.begin();
                auto it2 = set2.begin();
                while (it1 != set1.end() && it2 != set2.end()) {
                    if (*it1 == *it2) {
                        result.store(false, std::memory_order_relaxed);
                        return;
                    }
                    *it1 < *it2 ? ++it1 : ++it2;
                }
            }
        }
    };

    const unsigned num_threads = std::min<unsigned>(
        std::thread::hardware_concurrency(),
        active_states.size()
    );
    std::vector<std::future<void>> futures;
    futures.reserve(num_threads);
    const uint32_t base_chunk = active_states.size() / num_threads;
    const uint32_t remainder = active_states.size() % num_threads;
    uint32_t start = 0;
    for (unsigned i = 0; i < num_threads; ++i) {
        uint32_t end = start + base_chunk + (i < remainder ? 1 : 0);
        futures.emplace_back(std::async(std::launch::async, worker, start, end));
        start = end;
    }
    for (auto& future : futures) future.wait();
    return result.load(std::memory_order_relaxed);
}

static uint64_t countMemory(
    ShiftRegister reg,
    uint64_t upper_bound
) {
    uint64_t memory_size;
    IOSets sets = initIOSets(reg, upper_bound);
    for (memory_size = 1; memory_size <= upper_bound; ++memory_size) {
        //std::cerr << "countMemory memory_size = " << memory_size << std::endl << sets;
        std::cerr << "countMemory memory_size = " << memory_size << std::endl;
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
