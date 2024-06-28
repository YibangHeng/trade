#include <catch.hpp>
#include <thread>

#include "libreporter/AsyncReporter.h"

class CounterChecker final: public trade::reporter::NopReporter
{
public:
    explicit CounterChecker(const size_t sleep_time = 0)
        : m_sleep_time(sleep_time)
    {
    }
    ~CounterChecker() override = default;

public:
    void l2_tick_generated(const std::shared_ptr<trade::types::L2Tick> l2_tick) override
    {
        NopReporter::l2_tick_generated(l2_tick);

        /// Simulate time-consuming processing.
        std::this_thread::sleep_for(std::chrono::milliseconds(m_sleep_time));

        ++m_counter;
    }

public:
    [[nodiscard]] size_t get_trade_counter() const
    {
        return m_counter;
    }

private:
    std::atomic<size_t> m_counter;
    const size_t m_sleep_time;
};

TEST_CASE("AsyncReporter reporting", "[AsyncReporter]")
{
    SECTION("Huge reporting")
    {
        constexpr int iteration_times = 1000000;

        /// Report and count.
        const auto counter_checker = std::make_shared<CounterChecker>();
        auto reporter              = std::make_shared<trade::reporter::AsyncReporter>(counter_checker);

        for (int i = 0; i < iteration_times; i++) {
            reporter->l2_tick_generated(std::make_shared<trade::types::L2Tick>());
        }

        /// Wait for reporter to finish jobs.
        reporter.reset();

        CHECK(counter_checker->get_trade_counter() == iteration_times);
    }

    SECTION("Time consuming reporting")
    {
        constexpr int iteration_times = 10;

        /// Report and count.
        const auto counter_checker = std::make_shared<CounterChecker>(100); /// 10 milliseconds per job.
        auto reporter              = std::make_shared<trade::reporter::AsyncReporter>(counter_checker);

        for (int i = 0; i < iteration_times; i++) {
            reporter->l2_tick_generated(std::make_shared<trade::types::L2Tick>());
        }

        /// Wait for reporter to finish jobs.
        reporter.reset();

        CHECK(counter_checker->get_trade_counter() == iteration_times);
    }
}
