#include <catch.hpp>
#include <thread>

#include "utilities/TickerTaper.hpp"

TEST_CASE("TickerTaper", "[TickerTaper]")
{
    SECTION("TickerTaper increment")
    {
        trade::utilities::TickerTaper<uint64_t> ticker_taper;
        CHECK(ticker_taper() == 0);
        CHECK(ticker_taper() == 1);
        CHECK(ticker_taper() == 2);

        ticker_taper.reset();
        CHECK(ticker_taper() == 0);
        CHECK(ticker_taper() == 1);
        CHECK(ticker_taper() == 2);
    }

    SECTION("TickerTaper increment in multiple threads")
    {
        constexpr int num_threads     = 16;
        constexpr int iteration_times = 1024;

        trade::utilities::TickerTaper<uint64_t> ticker_taper;

        std::vector<std::thread> threads;
        threads.reserve(num_threads);
        for (int i = 0; i < num_threads; i++) {
            threads.emplace_back([&ticker_taper] {
                for (int j = 0; j < iteration_times; j++)
                    ticker_taper();
            });
        }
        for (auto& thread : threads)
            thread.join();

        CHECK(ticker_taper() == num_threads * iteration_times);
    }

    SECTION("TickerTaper for string increment")
    {
        trade::utilities::TickerTaper<std::string> ticker_taper;
        CHECK(ticker_taper() == "0");
        CHECK(ticker_taper() == "1");
        CHECK(ticker_taper() == "2");

        ticker_taper.reset();
        CHECK(ticker_taper() == "0");
        CHECK(ticker_taper() == "1");
        CHECK(ticker_taper() == "2");
    }

    SECTION("TickerTaper for string increment in multiple threads")
    {
        constexpr int num_threads     = 16;
        constexpr int iteration_times = 1024;

        trade::utilities::TickerTaper<std::string> ticker_taper;

        std::vector<std::thread> threads;
        threads.reserve(num_threads);
        for (int i = 0; i < num_threads; i++) {
            threads.emplace_back([&ticker_taper] {
                for (int j = 0; j < iteration_times; j++)
                    ticker_taper();
            });
        }
        for (auto& thread : threads)
            thread.join();

        CHECK(ticker_taper() == std::to_string(num_threads * iteration_times));
    }
}
