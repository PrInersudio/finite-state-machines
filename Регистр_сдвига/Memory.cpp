#include "Memory.hpp"
#include <tuple>
#include <iostream>
#include <thread>
#include <atomic>
#include <mutex>
#include <future>
#include "../Memory/IOSets.hpp"
#include "IOTuple.hpp"

static std::vector<std::unique_ptr<RedisContextWrapper>> ctxes;

static void initConnections(uint64_t num_states) {
    uint64_t count = std::min<uint64_t>(
        std::thread::hardware_concurrency(),
        num_states
    );
    ctxes.reserve(count);
    for (uint64_t i = 0; i < count; ++i) {
        ctxes.emplace_back(std::make_unique<RedisContextWrapper>());
    }
}

static bool delete_old_sets = true;

void disableOldSetsDeletion() {
    delete_old_sets = false;
}

static IOSets<IOTuple, uint32_t> initIOSets(MinimalShiftRegister &reg) {
    IOSets<IOTuple, uint32_t> sets(1);
    for (uint32_t state = 0; state <= static_cast<uint32_t>(reg.numStates() - 1); ++state)
        for (bool x : {false, true})
            sets.insert(*ctxes[0], reg.stateFunction(state, x), IOTuple(x, reg.outputFunction(state, x)));
    return sets;
}


static IOSets<IOTuple, uint32_t> updateIOSets(IOSets<IOTuple, uint32_t> &sets, MinimalShiftRegister &reg) {
    IOSets<IOTuple, uint32_t> new_sets(sets.getMemorySize() + 1);
    std::vector<uint32_t> active_states(sets.getActualStates().begin(), sets.getActualStates().end());

    auto worker = [&](uint32_t start, uint32_t end) {
        try {
            for (size_t i = start; i < end; ++i) {
                const uint32_t &state = active_states[i];
                for (auto it = sets.begin(*ctxes[i % ctxes.size()], state); it != sets.end(*ctxes[i % ctxes.size()], state); ++it)
                    for (bool x : {false, true}) {
                        IOTuple new_io = *it;
                        new_io.push(x, reg.outputFunction(state, x));
                        new_sets.insert(*ctxes[i % ctxes.size()], reg.stateFunction(state, x), new_io);
                    }
            }
        } catch (const std::exception &e) {
            std::cerr << std::string(e.what()) + "\n";
        }
    };

    const unsigned num_threads = std::min<unsigned>(
        ctxes.size(),
        active_states.size()
    );
    std::vector<std::future<void>> futures;
    futures.reserve(num_threads);
    const size_t chunk_size = (active_states.size() + num_threads - 1) / num_threads;
    size_t start = 0;
    for (unsigned i = 0; i < num_threads && start < active_states.size(); ++i) {
        size_t end = std::min(start + chunk_size, active_states.size());
        futures.emplace_back(std::async(std::launch::async, worker, start, end));
        start = end;
    }
    for (auto &future : futures)
        future.get();
    if(delete_old_sets) sets.clear(*ctxes[0]);
    return new_sets;
}

static bool checkMemoryCriteria(IOSets<IOTuple, uint32_t> &sets) {
    if (sets.empty()) return true;
    std::vector<uint32_t> active_states(sets.getActualStates().begin(), sets.getActualStates().end());
    if (active_states.size() < 2) return true;
    std::atomic<bool> result(true);

    auto worker = [&](uint32_t start, uint32_t end) {
        try {
            for (size_t i = start; i < end && result.load(std::memory_order_relaxed); ++i)
                for (size_t j = i + 1; j < active_states.size() && result.load(std::memory_order_relaxed); ++j) {
                    if (sets.intersects(*ctxes[i % ctxes.size()], active_states[i], active_states[j])) {
                        result.store(false, std::memory_order_relaxed);
                        break;
                    };
                }
        } catch (const std::exception &e) {
            std::cerr << std::string(e.what()) + "\n";
        }
    };

    const unsigned num_threads = std::min<unsigned>(
        ctxes.size(),
        active_states.size()
    );
    std::vector<std::future<void>> futures;
    futures.reserve(num_threads);
    const size_t total_work = active_states.size() - 1;
    const size_t chunk_size = (total_work + num_threads - 1) / num_threads;
    size_t start_i = 0;
    for (unsigned i = 0; i < num_threads && start_i < total_work; ++i) {
        size_t end_i = std::min(start_i + chunk_size, total_work);
        futures.emplace_back(std::async(std::launch::async, worker, start_i, end_i));
        start_i = end_i;
    }
    for (auto& future : futures) future.wait();
    return result.load(std::memory_order_relaxed);
}

static uint64_t countMemory(
    MinimalShiftRegister &reg,
    uint64_t upper_bound
) {
    uint64_t memory_size;
    initConnections(reg.numStates());
    IOSets sets = initIOSets(reg);
    for (memory_size = 1; memory_size <= upper_bound; ++memory_size) {
        std::cerr << "countMemory memory_size = " << memory_size << std::endl;
        if (checkMemoryCriteria(sets)) break;
        sets = std::move(updateIOSets(sets, reg));
    }
    if(delete_old_sets) sets.clear(*ctxes[0]);
    ctxes.clear();
    return memory_size;
}

void getMemoryShiftRegister(MinimalShiftRegister &reg) {
    uint64_t upper_bound = (reg.getMinimizedWeight() * (reg.getMinimizedWeight() - 1)) >> 1;
    uint64_t memory_size = upper_bound == 0 ? 0 : countMemory(reg, upper_bound);
    if (memory_size > upper_bound)
        std::cout << "Память автомата бесконечна." << std::endl;
    else
        std::cout << "Память автомата равна " << memory_size << std::endl;
}
