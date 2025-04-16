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

static IOSets<IOTuple, uint64_t> initIOSets(const LinearFSM &min) {
    IOSets<IOTuple, uint64_t> sets(1);
    for (const GFMatrix &state : min.stateRange())
        for (const GFMatrix &x : min.inputRange())
            sets.insert(*ctxes[0], uint64_t(min.stateFunction(state, x)), IOTuple(x, min.outputFunction(state, x)));
    return sets;
}


static IOSets<IOTuple, uint64_t> updateIOSets(IOSets<IOTuple, uint64_t> &sets, const LinearFSM &min) {
    IOSets<IOTuple, uint64_t> new_sets(sets.getMemorySize() + 1);
    std::vector<uint64_t> active_states(sets.getActualStates().begin(), sets.getActualStates().end());

    auto worker = [&](uint64_t start, uint64_t end) {
        try {
            for (uint64_t i = start; i < end; ++i) {
                const uint64_t &state = active_states[i];
                for (auto it = sets.begin(*ctxes[i % ctxes.size()], state); it != sets.end(*ctxes[i % ctxes.size()], state); ++it) {
                    const GFMatrix state_matrix = GFMatrix::FromIndex(min.getGF(), min.stateLength(), state);
                    for (const GFMatrix &x : min.inputRange()) {
                        IOTuple new_io = *it;
                        new_io.push(x, min.outputFunction(state_matrix, x));
                        new_sets.insert(*ctxes[i % ctxes.size()], uint64_t(min.stateFunction(state_matrix, x)), new_io);
                    }
                }
            }
        } catch (const std::exception &e) {
            std::cerr << std::string(e.what()) + "\n";
        }
    };

    const uint64_t num_threads = std::min<uint64_t>(
        ctxes.size(),
        active_states.size()
    );
    std::vector<std::future<void>> futures;
    futures.reserve(num_threads);
    const uint64_t chunk_size = (active_states.size() + num_threads - 1) / num_threads;
    uint64_t start = 0;
    for (uint64_t i = 0; i < num_threads && start < active_states.size(); ++i) {
        uint64_t end = std::min(static_cast<std::size_t>(start + chunk_size), active_states.size());
        futures.emplace_back(std::async(std::launch::async, worker, start, end));
        start = end;
    }
    for (auto &future : futures)
        future.get();
    if(delete_old_sets) sets.clear(*ctxes[0]);
    return new_sets;
}

static bool checkMemoryCriteria(IOSets<IOTuple, uint64_t> &sets) {
    if (sets.empty()) return true;
    std::vector<uint64_t> active_states(sets.getActualStates().begin(), sets.getActualStates().end());
    if (active_states.size() < 2) return true;
    std::atomic<bool> result(true);

    auto worker = [&](uint64_t start, uint64_t end) {
        try {
            for (uint64_t i = start; i < end && result.load(std::memory_order_relaxed); ++i)
                for (uint64_t j = i + 1; j < active_states.size() && result.load(std::memory_order_relaxed); ++j) {
                    if (sets.intersects(*ctxes[i % ctxes.size()], active_states[i], active_states[j])) {
                        //result.store(false, std::memory_order_relaxed);
                        break;
                    };
                }
        } catch (const std::exception &e) {
            std::cerr << std::string(e.what()) + "\n";
        }
    };

    const uint64_t num_threads = std::min<uint64_t>(
        ctxes.size(),
        active_states.size()
    );
    std::vector<std::future<void>> futures;
    futures.reserve(num_threads);
    const uint64_t total_work = active_states.size() - 1;
    const uint64_t chunk_size = (total_work + num_threads - 1) / num_threads;
    uint64_t start_i = 0;
    for (uint64_t i = 0; i < num_threads && start_i < total_work; ++i) {
        uint64_t end_i = std::min(start_i + chunk_size, total_work);
        futures.emplace_back(std::async(std::launch::async, worker, start_i, end_i));
        start_i = end_i;
    }
    for (auto& future : futures) future.wait();
    return result.load(std::memory_order_relaxed);
}

static uint64_t countMemory(
    const LinearFSM &min,
    uint64_t upper_bound
) {
    uint64_t memory_size;
    initConnections(min.numStates());
    IOSets sets = initIOSets(min);
    for (memory_size = 1; memory_size <= upper_bound; ++memory_size) {
        std::cerr << "countMemory memory_size = " << memory_size << std::endl;
        if (checkMemoryCriteria(sets)) break;
        std::cerr << "countMemory memory_size = " << memory_size << std::endl;
        sets = std::move(updateIOSets(sets, min));
    }
    if(delete_old_sets) sets.clear(*ctxes[0]);
    ctxes.clear();
    return memory_size;
}

void getMemoryLinearFSM(const LinearFSM &min) {
    uint64_t upper_bound = min.getMemoryUpperBound();
    uint64_t memory_size = upper_bound == 0 ? 0 : countMemory(min, upper_bound);
    if (memory_size > upper_bound)
        std::cout << "Память автомата бесконечна." << std::endl;
    else
        std::cout << "Память автомата равна " << memory_size << std::endl;
}
