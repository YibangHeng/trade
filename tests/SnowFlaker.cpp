#include <catch.hpp>
#include <thread>

#include "utilities/SnowFlaker.hpp"

TEST_CASE("UUID generating", "[SnowFlaker]")
{
    constexpr int num_threads     = 16;
    constexpr int iteration_times = 100000;

    std::vector<std::thread> threads;
    threads.reserve(num_threads);
    std::vector<std::set<int64_t>> id_set {num_threads};

    auto generator = [](std::set<int64_t>& ids) {
        for (int j = 0; j < iteration_times; j++) {
            ids.insert(NEW_ID());
        }
    };

    for (int i = 0; i < num_threads; i++) {
        threads.emplace_back(generator, std::ref(id_set[i]));
    }

    for (auto& thread : threads) {
        thread.join();
    }

    std::set<int64_t> all_ids;
    for (auto& id : id_set) {
        all_ids.insert(id.begin(), id.end());
    }

    /// Check whether all IDs are unique.
    CHECK(all_ids.size() == num_threads * iteration_times);
}
