#include <catch.hpp>
#include <thread>

#include "utilities/SeqChecker.hpp"

TEST_CASE("SeqChecker", "[SeqChecker]")
{
    constexpr int iteration_times = 1000000;

    SECTION("Check without missing")
    {
        trade::utilities::SeqChecker<uint64_t> seqChecker(0);

        for (uint64_t i = 1; i < iteration_times; i++) {
            CHECK(seqChecker.diff(i) == 1);
        }
    }

    SECTION("Check with missing")
    {
        trade::utilities::SeqChecker<uint64_t> seqChecker(-1);

        for (uint64_t i = 1; i < iteration_times; i += i % 2 + 1) {
            if (i % 2 == 0)
                CHECK(seqChecker.diff(i) == 1);
            else
                CHECK(seqChecker.diff(i) == 2);
        }
    }
}
