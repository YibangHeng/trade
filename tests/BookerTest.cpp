#include <catch.hpp>

#include "libbooker/Booker.h"
#include "libreporter/NopReporter.hpp"
#include "utilities/TickCreator.hpp"

class SeqChecker final: public trade::reporter::NopReporter
{
public:
    SeqChecker()           = default;
    ~SeqChecker() override = default;

public:
    void reset()
    {
        m_trade_results.clear();
    }

public:
    [[nodiscard]] auto get_trade_result() const
    {
        return m_trade_results;
    }

public:
    void l2_tick_generated(const trade::booker::GeneratedL2TickPtr generated_l2_tick) override
    {
        m_trade_results.push_back(generated_l2_tick);
    }

private:
    std::vector<trade::booker::GeneratedL2TickPtr> m_trade_results;
};

const auto g_reporter = std::make_shared<SeqChecker>();

/// For reusing the same reporter.
const auto reporter = [] {g_reporter->reset(); return g_reporter; };

TEST_CASE("Booker system correctness verification", "[Booker]")
{
    SECTION("Limit order matching with 1:1 matching")
    {
        SECTION("Sell after buy with same price")
        {
            trade::booker::Booker booker({}, reporter());

            booker.add(TickCreator::order_tick(0, LIMIT, "600875.SH", BUY, 2233, 100));
            booker.add(TickCreator::order_tick(1, LIMIT, "600875.SH", SELL, 2233, 100));

            CHECK(g_reporter->get_trade_result().size() == 1);

            CHECK(g_reporter->get_trade_result()[0]->symbol() == "600875.SH");
            CHECK(g_reporter->get_trade_result()[0]->price_1000x() == 2233);
            CHECK(g_reporter->get_trade_result()[0]->quantity() == 100);
            CHECK(g_reporter->get_trade_result()[0]->ask_unique_id() == 1);
            CHECK(g_reporter->get_trade_result()[0]->bid_unique_id() == 0);
            CHECK(g_reporter->get_trade_result()[0]->sell_price_1000x_3() == 0);
            CHECK(g_reporter->get_trade_result()[0]->sell_price_1000x_2() == 0);
            CHECK(g_reporter->get_trade_result()[0]->sell_price_1000x_1() == 0);
            CHECK(g_reporter->get_trade_result()[0]->buy_price_1000x_1() == 0);
            CHECK(g_reporter->get_trade_result()[0]->buy_price_1000x_2() == 0);
            CHECK(g_reporter->get_trade_result()[0]->buy_price_1000x_3() == 0);
            CHECK(g_reporter->get_trade_result()[0]->sell_quantity_3() == 0);
            CHECK(g_reporter->get_trade_result()[0]->sell_quantity_2() == 0);
            CHECK(g_reporter->get_trade_result()[0]->sell_quantity_1() == 0);
            CHECK(g_reporter->get_trade_result()[0]->buy_quantity_1() == 0);
            CHECK(g_reporter->get_trade_result()[0]->buy_quantity_2() == 0);
            CHECK(g_reporter->get_trade_result()[0]->buy_quantity_3() == 0);
        }

        SECTION("Buy after sell with same price")
        {
            trade::booker::Booker booker({}, reporter());

            booker.add(TickCreator::order_tick(0, LIMIT, "600875.SH", SELL, 2233, 100));
            booker.add(TickCreator::order_tick(1, LIMIT, "600875.SH", BUY, 2233, 100));

            CHECK(g_reporter->get_trade_result().size() == 1);

            CHECK(g_reporter->get_trade_result()[0]->symbol() == "600875.SH");
            CHECK(g_reporter->get_trade_result()[0]->price_1000x() == 2233);
            CHECK(g_reporter->get_trade_result()[0]->quantity() == 100);
            CHECK(g_reporter->get_trade_result()[0]->ask_unique_id() == 0);
            CHECK(g_reporter->get_trade_result()[0]->bid_unique_id() == 1);
            CHECK(g_reporter->get_trade_result()[0]->sell_price_1000x_3() == 0);
            CHECK(g_reporter->get_trade_result()[0]->sell_price_1000x_2() == 0);
            CHECK(g_reporter->get_trade_result()[0]->sell_price_1000x_1() == 0);
            CHECK(g_reporter->get_trade_result()[0]->buy_price_1000x_1() == 0);
            CHECK(g_reporter->get_trade_result()[0]->buy_price_1000x_2() == 0);
            CHECK(g_reporter->get_trade_result()[0]->buy_price_1000x_3() == 0);
            CHECK(g_reporter->get_trade_result()[0]->sell_quantity_3() == 0);
            CHECK(g_reporter->get_trade_result()[0]->sell_quantity_2() == 0);
            CHECK(g_reporter->get_trade_result()[0]->sell_quantity_1() == 0);
            CHECK(g_reporter->get_trade_result()[0]->buy_quantity_1() == 0);
            CHECK(g_reporter->get_trade_result()[0]->buy_quantity_2() == 0);
            CHECK(g_reporter->get_trade_result()[0]->buy_quantity_3() == 0);
        }

        SECTION("Sell after buy with lower price")
        {
            trade::booker::Booker booker({}, reporter());

            booker.add(TickCreator::order_tick(0, LIMIT, "600875.SH", BUY, 3322, 100));
            booker.add(TickCreator::order_tick(1, LIMIT, "600875.SH", SELL, 2233, 100));

            CHECK(g_reporter->get_trade_result().size() == 1);

            CHECK(g_reporter->get_trade_result()[0]->symbol() == "600875.SH");
            CHECK(g_reporter->get_trade_result()[0]->price_1000x() == 3322);
            CHECK(g_reporter->get_trade_result()[0]->quantity() == 100);
            CHECK(g_reporter->get_trade_result()[0]->ask_unique_id() == 1);
            CHECK(g_reporter->get_trade_result()[0]->bid_unique_id() == 0);
            CHECK(g_reporter->get_trade_result()[0]->sell_price_1000x_3() == 0);
            CHECK(g_reporter->get_trade_result()[0]->sell_price_1000x_2() == 0);
            CHECK(g_reporter->get_trade_result()[0]->sell_price_1000x_1() == 0);
            CHECK(g_reporter->get_trade_result()[0]->buy_price_1000x_1() == 0);
            CHECK(g_reporter->get_trade_result()[0]->buy_price_1000x_2() == 0);
            CHECK(g_reporter->get_trade_result()[0]->buy_price_1000x_3() == 0);
            CHECK(g_reporter->get_trade_result()[0]->sell_quantity_3() == 0);
            CHECK(g_reporter->get_trade_result()[0]->sell_quantity_2() == 0);
            CHECK(g_reporter->get_trade_result()[0]->sell_quantity_1() == 0);
            CHECK(g_reporter->get_trade_result()[0]->buy_quantity_1() == 0);
            CHECK(g_reporter->get_trade_result()[0]->buy_quantity_2() == 0);
            CHECK(g_reporter->get_trade_result()[0]->buy_quantity_3() == 0);
        }

        SECTION("Buy after sell with higher price")
        {
            trade::booker::Booker booker({}, reporter());

            booker.add(TickCreator::order_tick(0, LIMIT, "600875.SH", SELL, 2233, 100));
            booker.add(TickCreator::order_tick(1, LIMIT, "600875.SH", BUY, 3322, 100));

            CHECK(g_reporter->get_trade_result().size() == 1);

            CHECK(g_reporter->get_trade_result()[0]->symbol() == "600875.SH");
            CHECK(g_reporter->get_trade_result()[0]->price_1000x() == 2233);
            CHECK(g_reporter->get_trade_result()[0]->quantity() == 100);
            CHECK(g_reporter->get_trade_result()[0]->ask_unique_id() == 0);
            CHECK(g_reporter->get_trade_result()[0]->bid_unique_id() == 1);
            CHECK(g_reporter->get_trade_result()[0]->sell_price_1000x_3() == 0);
            CHECK(g_reporter->get_trade_result()[0]->sell_price_1000x_2() == 0);
            CHECK(g_reporter->get_trade_result()[0]->sell_price_1000x_1() == 0);
            CHECK(g_reporter->get_trade_result()[0]->buy_price_1000x_1() == 0);
            CHECK(g_reporter->get_trade_result()[0]->buy_price_1000x_2() == 0);
            CHECK(g_reporter->get_trade_result()[0]->buy_price_1000x_3() == 0);
            CHECK(g_reporter->get_trade_result()[0]->sell_quantity_3() == 0);
            CHECK(g_reporter->get_trade_result()[0]->sell_quantity_2() == 0);
            CHECK(g_reporter->get_trade_result()[0]->sell_quantity_1() == 0);
            CHECK(g_reporter->get_trade_result()[0]->buy_quantity_1() == 0);
            CHECK(g_reporter->get_trade_result()[0]->buy_quantity_2() == 0);
            CHECK(g_reporter->get_trade_result()[0]->buy_quantity_3() == 0);
        }

        SECTION("Sell after buy with higher price")
        {
            trade::booker::Booker booker({}, reporter());

            booker.add(TickCreator::order_tick(0, LIMIT, "600875.SH", BUY, 2233, 100));
            booker.add(TickCreator::order_tick(1, LIMIT, "600875.SH", SELL, 3322, 100));

            CHECK(g_reporter->get_trade_result().empty());
        }

        SECTION("Buy after sell with lower price")
        {
            trade::booker::Booker booker({}, reporter());

            booker.add(TickCreator::order_tick(0, LIMIT, "600875.SH", SELL, 3322, 100));
            booker.add(TickCreator::order_tick(1, LIMIT, "600875.SH", BUY, 2233, 100));

            CHECK(g_reporter->get_trade_result().empty());
        }
    }

    SECTION("Limit order matching with M:N matching")
    {
        SECTION("Buy after sell with same price")
        {
            trade::booker::Booker booker({}, reporter());

            booker.add(TickCreator::order_tick(0, LIMIT, "600875.SH", SELL, 2233, 20));
            booker.add(TickCreator::order_tick(1, LIMIT, "600875.SH", SELL, 2233, 40));
            booker.add(TickCreator::order_tick(2, LIMIT, "600875.SH", SELL, 2233, 80));
            booker.add(TickCreator::order_tick(3, LIMIT, "600875.SH", BUY, 2233, 100));
            booker.add(TickCreator::order_tick(4, LIMIT, "600875.SH", BUY, 2233, 100));

            CHECK(g_reporter->get_trade_result().size() == 4);

            CHECK(g_reporter->get_trade_result()[0]->symbol() == "600875.SH");
            CHECK(g_reporter->get_trade_result()[0]->price_1000x() == 2233);
            CHECK(g_reporter->get_trade_result()[0]->quantity() == 20);
            CHECK(g_reporter->get_trade_result()[0]->ask_unique_id() == 0);
            CHECK(g_reporter->get_trade_result()[0]->bid_unique_id() == 3);
            CHECK(g_reporter->get_trade_result()[0]->sell_price_1000x_3() == 0);
            CHECK(g_reporter->get_trade_result()[0]->sell_price_1000x_2() == 0);
            CHECK(g_reporter->get_trade_result()[0]->sell_price_1000x_1() == 2233);
            CHECK(g_reporter->get_trade_result()[0]->buy_price_1000x_1() == 0);
            CHECK(g_reporter->get_trade_result()[0]->buy_price_1000x_2() == 0);
            CHECK(g_reporter->get_trade_result()[0]->buy_price_1000x_3() == 0);
            CHECK(g_reporter->get_trade_result()[0]->sell_quantity_3() == 0);
            CHECK(g_reporter->get_trade_result()[0]->sell_quantity_2() == 0);
            CHECK(g_reporter->get_trade_result()[0]->sell_quantity_1() == 40);
            CHECK(g_reporter->get_trade_result()[0]->buy_quantity_1() == 0);
            CHECK(g_reporter->get_trade_result()[0]->buy_quantity_2() == 0);
            CHECK(g_reporter->get_trade_result()[0]->buy_quantity_3() == 0);

            CHECK(g_reporter->get_trade_result()[1]->symbol() == "600875.SH");
            CHECK(g_reporter->get_trade_result()[1]->price_1000x() == 2233);
            CHECK(g_reporter->get_trade_result()[1]->quantity() == 40);
            CHECK(g_reporter->get_trade_result()[1]->ask_unique_id() == 1);
            CHECK(g_reporter->get_trade_result()[1]->bid_unique_id() == 3);
            CHECK(g_reporter->get_trade_result()[1]->sell_price_1000x_3() == 0);
            CHECK(g_reporter->get_trade_result()[1]->sell_price_1000x_2() == 0);
            CHECK(g_reporter->get_trade_result()[1]->sell_price_1000x_1() == 2233);
            CHECK(g_reporter->get_trade_result()[1]->buy_price_1000x_1() == 0);
            CHECK(g_reporter->get_trade_result()[1]->buy_price_1000x_2() == 0);
            CHECK(g_reporter->get_trade_result()[1]->buy_price_1000x_3() == 0);
            CHECK(g_reporter->get_trade_result()[1]->sell_quantity_3() == 0);
            CHECK(g_reporter->get_trade_result()[1]->sell_quantity_2() == 0);
            CHECK(g_reporter->get_trade_result()[1]->sell_quantity_1() == 40);
            CHECK(g_reporter->get_trade_result()[1]->buy_quantity_1() == 0);
            CHECK(g_reporter->get_trade_result()[1]->buy_quantity_2() == 0);
            CHECK(g_reporter->get_trade_result()[1]->buy_quantity_3() == 0);

            CHECK(g_reporter->get_trade_result()[2]->symbol() == "600875.SH");
            CHECK(g_reporter->get_trade_result()[2]->price_1000x() == 2233);
            CHECK(g_reporter->get_trade_result()[2]->quantity() == 40);
            CHECK(g_reporter->get_trade_result()[2]->ask_unique_id() == 2);
            CHECK(g_reporter->get_trade_result()[2]->bid_unique_id() == 3);
            CHECK(g_reporter->get_trade_result()[2]->sell_price_1000x_3() == 0);
            CHECK(g_reporter->get_trade_result()[2]->sell_price_1000x_2() == 0);
            CHECK(g_reporter->get_trade_result()[2]->sell_price_1000x_1() == 2233);
            CHECK(g_reporter->get_trade_result()[2]->buy_price_1000x_1() == 0);
            CHECK(g_reporter->get_trade_result()[2]->buy_price_1000x_2() == 0);
            CHECK(g_reporter->get_trade_result()[2]->buy_price_1000x_3() == 0);
            CHECK(g_reporter->get_trade_result()[2]->sell_quantity_3() == 0);
            CHECK(g_reporter->get_trade_result()[2]->sell_quantity_2() == 0);
            CHECK(g_reporter->get_trade_result()[2]->sell_quantity_1() == 40);
            CHECK(g_reporter->get_trade_result()[2]->buy_quantity_1() == 0);
            CHECK(g_reporter->get_trade_result()[2]->buy_quantity_2() == 0);
            CHECK(g_reporter->get_trade_result()[2]->buy_quantity_3() == 0);

            CHECK(g_reporter->get_trade_result()[3]->symbol() == "600875.SH");
            CHECK(g_reporter->get_trade_result()[3]->price_1000x() == 2233);
            CHECK(g_reporter->get_trade_result()[3]->quantity() == 40);
            CHECK(g_reporter->get_trade_result()[3]->ask_unique_id() == 2);
            CHECK(g_reporter->get_trade_result()[3]->bid_unique_id() == 4);
            CHECK(g_reporter->get_trade_result()[3]->sell_price_1000x_3() == 0);
            CHECK(g_reporter->get_trade_result()[3]->sell_price_1000x_2() == 0);
            CHECK(g_reporter->get_trade_result()[3]->sell_price_1000x_1() == 0);
            CHECK(g_reporter->get_trade_result()[3]->buy_price_1000x_1() == 2233);
            CHECK(g_reporter->get_trade_result()[3]->buy_price_1000x_2() == 0);
            CHECK(g_reporter->get_trade_result()[3]->buy_price_1000x_3() == 0);
            CHECK(g_reporter->get_trade_result()[3]->sell_quantity_3() == 0);
            CHECK(g_reporter->get_trade_result()[3]->sell_quantity_2() == 0);
            CHECK(g_reporter->get_trade_result()[3]->sell_quantity_1() == 0);
            CHECK(g_reporter->get_trade_result()[3]->buy_quantity_1() == 60);
            CHECK(g_reporter->get_trade_result()[3]->buy_quantity_2() == 0);
            CHECK(g_reporter->get_trade_result()[3]->buy_quantity_3() == 0);
        }

        SECTION("Buy after sell with same price")
        {
            trade::booker::Booker booker({}, reporter());

            booker.add(TickCreator::order_tick(0, LIMIT, "600875.SH", SELL, 2233, 20));
            booker.add(TickCreator::order_tick(1, LIMIT, "600875.SH", SELL, 2233, 40));
            booker.add(TickCreator::order_tick(2, LIMIT, "600875.SH", SELL, 2233, 80));
            booker.add(TickCreator::order_tick(3, LIMIT, "600875.SH", BUY, 2233, 100));
            booker.add(TickCreator::order_tick(4, LIMIT, "600875.SH", BUY, 2233, 100));

            CHECK(g_reporter->get_trade_result().size() == 4);

            CHECK(g_reporter->get_trade_result()[0]->symbol() == "600875.SH");
            CHECK(g_reporter->get_trade_result()[0]->price_1000x() == 2233);
            CHECK(g_reporter->get_trade_result()[0]->quantity() == 20);
            CHECK(g_reporter->get_trade_result()[0]->ask_unique_id() == 0);
            CHECK(g_reporter->get_trade_result()[0]->bid_unique_id() == 3);
            CHECK(g_reporter->get_trade_result()[0]->sell_price_1000x_3() == 0);
            CHECK(g_reporter->get_trade_result()[0]->sell_price_1000x_2() == 0);
            CHECK(g_reporter->get_trade_result()[0]->sell_price_1000x_1() == 2233);
            CHECK(g_reporter->get_trade_result()[0]->buy_price_1000x_1() == 0);
            CHECK(g_reporter->get_trade_result()[0]->buy_price_1000x_2() == 0);
            CHECK(g_reporter->get_trade_result()[0]->buy_price_1000x_3() == 0);
            CHECK(g_reporter->get_trade_result()[0]->sell_quantity_3() == 0);
            CHECK(g_reporter->get_trade_result()[0]->sell_quantity_2() == 0);
            CHECK(g_reporter->get_trade_result()[0]->sell_quantity_1() == 40);
            CHECK(g_reporter->get_trade_result()[0]->buy_quantity_1() == 0);
            CHECK(g_reporter->get_trade_result()[0]->buy_quantity_2() == 0);
            CHECK(g_reporter->get_trade_result()[0]->buy_quantity_3() == 0);

            CHECK(g_reporter->get_trade_result()[1]->symbol() == "600875.SH");
            CHECK(g_reporter->get_trade_result()[1]->price_1000x() == 2233);
            CHECK(g_reporter->get_trade_result()[1]->quantity() == 40);
            CHECK(g_reporter->get_trade_result()[1]->ask_unique_id() == 1);
            CHECK(g_reporter->get_trade_result()[1]->bid_unique_id() == 3);
            CHECK(g_reporter->get_trade_result()[1]->sell_price_1000x_3() == 0);
            CHECK(g_reporter->get_trade_result()[1]->sell_price_1000x_2() == 0);
            CHECK(g_reporter->get_trade_result()[1]->sell_price_1000x_1() == 2233);
            CHECK(g_reporter->get_trade_result()[1]->buy_price_1000x_1() == 0);
            CHECK(g_reporter->get_trade_result()[1]->buy_price_1000x_2() == 0);
            CHECK(g_reporter->get_trade_result()[1]->buy_price_1000x_3() == 0);
            CHECK(g_reporter->get_trade_result()[1]->sell_quantity_3() == 0);
            CHECK(g_reporter->get_trade_result()[1]->sell_quantity_2() == 0);
            CHECK(g_reporter->get_trade_result()[1]->sell_quantity_1() == 40);
            CHECK(g_reporter->get_trade_result()[1]->buy_quantity_1() == 0);
            CHECK(g_reporter->get_trade_result()[1]->buy_quantity_2() == 0);
            CHECK(g_reporter->get_trade_result()[1]->buy_quantity_3() == 0);

            CHECK(g_reporter->get_trade_result()[2]->symbol() == "600875.SH");
            CHECK(g_reporter->get_trade_result()[2]->price_1000x() == 2233);
            CHECK(g_reporter->get_trade_result()[2]->quantity() == 40);
            CHECK(g_reporter->get_trade_result()[2]->ask_unique_id() == 2);
            CHECK(g_reporter->get_trade_result()[2]->bid_unique_id() == 3);
            CHECK(g_reporter->get_trade_result()[2]->sell_price_1000x_3() == 0);
            CHECK(g_reporter->get_trade_result()[2]->sell_price_1000x_2() == 0);
            CHECK(g_reporter->get_trade_result()[2]->sell_price_1000x_1() == 2233);
            CHECK(g_reporter->get_trade_result()[2]->buy_price_1000x_1() == 0);
            CHECK(g_reporter->get_trade_result()[2]->buy_price_1000x_2() == 0);
            CHECK(g_reporter->get_trade_result()[2]->buy_price_1000x_3() == 0);
            CHECK(g_reporter->get_trade_result()[2]->sell_quantity_3() == 0);
            CHECK(g_reporter->get_trade_result()[2]->sell_quantity_2() == 0);
            CHECK(g_reporter->get_trade_result()[2]->sell_quantity_1() == 40);
            CHECK(g_reporter->get_trade_result()[2]->buy_quantity_1() == 0);
            CHECK(g_reporter->get_trade_result()[2]->buy_quantity_2() == 0);
            CHECK(g_reporter->get_trade_result()[2]->buy_quantity_3() == 0);

            CHECK(g_reporter->get_trade_result()[3]->symbol() == "600875.SH");
            CHECK(g_reporter->get_trade_result()[3]->price_1000x() == 2233);
            CHECK(g_reporter->get_trade_result()[3]->quantity() == 40);
            CHECK(g_reporter->get_trade_result()[3]->ask_unique_id() == 2);
            CHECK(g_reporter->get_trade_result()[3]->bid_unique_id() == 4);
            CHECK(g_reporter->get_trade_result()[3]->sell_price_1000x_3() == 0);
            CHECK(g_reporter->get_trade_result()[3]->sell_price_1000x_2() == 0);
            CHECK(g_reporter->get_trade_result()[3]->sell_price_1000x_1() == 0);
            CHECK(g_reporter->get_trade_result()[3]->buy_price_1000x_1() == 2233);
            CHECK(g_reporter->get_trade_result()[3]->buy_price_1000x_2() == 0);
            CHECK(g_reporter->get_trade_result()[3]->buy_price_1000x_3() == 0);
            CHECK(g_reporter->get_trade_result()[3]->sell_quantity_3() == 0);
            CHECK(g_reporter->get_trade_result()[3]->sell_quantity_2() == 0);
            CHECK(g_reporter->get_trade_result()[3]->sell_quantity_1() == 0);
            CHECK(g_reporter->get_trade_result()[3]->buy_quantity_1() == 60);
            CHECK(g_reporter->get_trade_result()[3]->buy_quantity_2() == 0);
            CHECK(g_reporter->get_trade_result()[3]->buy_quantity_3() == 0);
        }

        SECTION("Sell after buy with lower price")
        {
            trade::booker::Booker booker({}, reporter());

            booker.add(TickCreator::order_tick(0, LIMIT, "600875.SH", BUY, 3322, 20));
            booker.add(TickCreator::order_tick(1, LIMIT, "600875.SH", BUY, 3322, 40));
            booker.add(TickCreator::order_tick(2, LIMIT, "600875.SH", BUY, 3322, 80));
            booker.add(TickCreator::order_tick(3, LIMIT, "600875.SH", SELL, 2233, 100));
            booker.add(TickCreator::order_tick(4, LIMIT, "600875.SH", SELL, 2233, 100));

            CHECK(g_reporter->get_trade_result().size() == 4);

            CHECK(g_reporter->get_trade_result()[0]->symbol() == "600875.SH");
            CHECK(g_reporter->get_trade_result()[0]->price_1000x() == 3322);
            CHECK(g_reporter->get_trade_result()[0]->quantity() == 20);
            CHECK(g_reporter->get_trade_result()[0]->ask_unique_id() == 3);
            CHECK(g_reporter->get_trade_result()[0]->bid_unique_id() == 0);
            CHECK(g_reporter->get_trade_result()[0]->sell_price_1000x_3() == 0);
            CHECK(g_reporter->get_trade_result()[0]->sell_price_1000x_2() == 0);
            CHECK(g_reporter->get_trade_result()[0]->sell_price_1000x_1() == 0);
            CHECK(g_reporter->get_trade_result()[0]->buy_price_1000x_1() == 3322);
            CHECK(g_reporter->get_trade_result()[0]->buy_price_1000x_2() == 0);
            CHECK(g_reporter->get_trade_result()[0]->buy_price_1000x_3() == 0);
            CHECK(g_reporter->get_trade_result()[0]->sell_quantity_3() == 0);
            CHECK(g_reporter->get_trade_result()[0]->sell_quantity_2() == 0);
            CHECK(g_reporter->get_trade_result()[0]->sell_quantity_1() == 0);
            CHECK(g_reporter->get_trade_result()[0]->buy_quantity_1() == 40);
            CHECK(g_reporter->get_trade_result()[0]->buy_quantity_2() == 0);
            CHECK(g_reporter->get_trade_result()[0]->buy_quantity_3() == 0);

            CHECK(g_reporter->get_trade_result()[1]->symbol() == "600875.SH");
            CHECK(g_reporter->get_trade_result()[1]->price_1000x() == 3322);
            CHECK(g_reporter->get_trade_result()[1]->quantity() == 40);
            CHECK(g_reporter->get_trade_result()[1]->ask_unique_id() == 3);
            CHECK(g_reporter->get_trade_result()[1]->bid_unique_id() == 1);
            CHECK(g_reporter->get_trade_result()[1]->sell_price_1000x_3() == 0);
            CHECK(g_reporter->get_trade_result()[1]->sell_price_1000x_2() == 0);
            CHECK(g_reporter->get_trade_result()[1]->sell_price_1000x_1() == 0);
            CHECK(g_reporter->get_trade_result()[1]->buy_price_1000x_1() == 3322);
            CHECK(g_reporter->get_trade_result()[1]->buy_price_1000x_2() == 0);
            CHECK(g_reporter->get_trade_result()[1]->buy_price_1000x_3() == 0);
            CHECK(g_reporter->get_trade_result()[1]->sell_quantity_3() == 0);
            CHECK(g_reporter->get_trade_result()[1]->sell_quantity_2() == 0);
            CHECK(g_reporter->get_trade_result()[1]->sell_quantity_1() == 0);
            CHECK(g_reporter->get_trade_result()[1]->buy_quantity_1() == 40);
            CHECK(g_reporter->get_trade_result()[1]->buy_quantity_2() == 0);
            CHECK(g_reporter->get_trade_result()[1]->buy_quantity_3() == 0);

            CHECK(g_reporter->get_trade_result()[2]->symbol() == "600875.SH");
            CHECK(g_reporter->get_trade_result()[2]->price_1000x() == 3322);
            CHECK(g_reporter->get_trade_result()[2]->quantity() == 40);
            CHECK(g_reporter->get_trade_result()[2]->ask_unique_id() == 3);
            CHECK(g_reporter->get_trade_result()[2]->bid_unique_id() == 2);
            CHECK(g_reporter->get_trade_result()[2]->sell_price_1000x_3() == 0);
            CHECK(g_reporter->get_trade_result()[2]->sell_price_1000x_2() == 0);
            CHECK(g_reporter->get_trade_result()[2]->sell_price_1000x_1() == 0);
            CHECK(g_reporter->get_trade_result()[2]->buy_price_1000x_1() == 3322);
            CHECK(g_reporter->get_trade_result()[2]->buy_price_1000x_2() == 0);
            CHECK(g_reporter->get_trade_result()[2]->buy_price_1000x_3() == 0);
            CHECK(g_reporter->get_trade_result()[2]->sell_quantity_3() == 0);
            CHECK(g_reporter->get_trade_result()[2]->sell_quantity_2() == 0);
            CHECK(g_reporter->get_trade_result()[2]->sell_quantity_1() == 0);
            CHECK(g_reporter->get_trade_result()[2]->buy_quantity_1() == 40);
            CHECK(g_reporter->get_trade_result()[2]->buy_quantity_2() == 0);
            CHECK(g_reporter->get_trade_result()[2]->buy_quantity_3() == 0);

            CHECK(g_reporter->get_trade_result()[3]->symbol() == "600875.SH");
            CHECK(g_reporter->get_trade_result()[3]->price_1000x() == 3322);
            CHECK(g_reporter->get_trade_result()[3]->quantity() == 40);
            CHECK(g_reporter->get_trade_result()[3]->ask_unique_id() == 4);
            CHECK(g_reporter->get_trade_result()[3]->bid_unique_id() == 2);
            CHECK(g_reporter->get_trade_result()[3]->sell_price_1000x_3() == 0);
            CHECK(g_reporter->get_trade_result()[3]->sell_price_1000x_2() == 0);
            CHECK(g_reporter->get_trade_result()[3]->sell_price_1000x_1() == 2233);
            CHECK(g_reporter->get_trade_result()[3]->buy_price_1000x_1() == 0);
            CHECK(g_reporter->get_trade_result()[3]->buy_price_1000x_2() == 0);
            CHECK(g_reporter->get_trade_result()[3]->buy_price_1000x_3() == 0);
            CHECK(g_reporter->get_trade_result()[3]->sell_quantity_3() == 0);
            CHECK(g_reporter->get_trade_result()[3]->sell_quantity_2() == 0);
            CHECK(g_reporter->get_trade_result()[3]->sell_quantity_1() == 60);
            CHECK(g_reporter->get_trade_result()[3]->buy_quantity_1() == 0);
            CHECK(g_reporter->get_trade_result()[3]->buy_quantity_2() == 0);
            CHECK(g_reporter->get_trade_result()[3]->buy_quantity_3() == 0);
        }

        SECTION("Buy after sell with higher price")
        {
            trade::booker::Booker booker({}, reporter());

            booker.add(TickCreator::order_tick(0, LIMIT, "600875.SH", SELL, 2233, 20));
            booker.add(TickCreator::order_tick(1, LIMIT, "600875.SH", SELL, 2233, 40));
            booker.add(TickCreator::order_tick(2, LIMIT, "600875.SH", SELL, 2233, 80));
            booker.add(TickCreator::order_tick(3, LIMIT, "600875.SH", BUY, 3322, 100));
            booker.add(TickCreator::order_tick(4, LIMIT, "600875.SH", BUY, 3322, 100));

            CHECK(g_reporter->get_trade_result().size() == 4);

            CHECK(g_reporter->get_trade_result()[0]->symbol() == "600875.SH");
            CHECK(g_reporter->get_trade_result()[0]->price_1000x() == 2233);
            CHECK(g_reporter->get_trade_result()[0]->quantity() == 20);
            CHECK(g_reporter->get_trade_result()[0]->ask_unique_id() == 0);
            CHECK(g_reporter->get_trade_result()[0]->bid_unique_id() == 3);
            CHECK(g_reporter->get_trade_result()[0]->sell_price_1000x_3() == 0);
            CHECK(g_reporter->get_trade_result()[0]->sell_price_1000x_2() == 0);
            CHECK(g_reporter->get_trade_result()[0]->sell_price_1000x_1() == 2233);
            CHECK(g_reporter->get_trade_result()[0]->buy_price_1000x_1() == 0);
            CHECK(g_reporter->get_trade_result()[0]->buy_price_1000x_2() == 0);
            CHECK(g_reporter->get_trade_result()[0]->buy_price_1000x_3() == 0);
            CHECK(g_reporter->get_trade_result()[0]->sell_quantity_3() == 0);
            CHECK(g_reporter->get_trade_result()[0]->sell_quantity_2() == 0);
            CHECK(g_reporter->get_trade_result()[0]->sell_quantity_1() == 40);
            CHECK(g_reporter->get_trade_result()[0]->buy_quantity_1() == 0);
            CHECK(g_reporter->get_trade_result()[0]->buy_quantity_2() == 0);
            CHECK(g_reporter->get_trade_result()[0]->buy_quantity_3() == 0);

            CHECK(g_reporter->get_trade_result()[1]->symbol() == "600875.SH");
            CHECK(g_reporter->get_trade_result()[1]->price_1000x() == 2233);
            CHECK(g_reporter->get_trade_result()[1]->quantity() == 40);
            CHECK(g_reporter->get_trade_result()[1]->ask_unique_id() == 1);
            CHECK(g_reporter->get_trade_result()[1]->bid_unique_id() == 3);
            CHECK(g_reporter->get_trade_result()[1]->sell_price_1000x_3() == 0);
            CHECK(g_reporter->get_trade_result()[1]->sell_price_1000x_2() == 0);
            CHECK(g_reporter->get_trade_result()[1]->sell_price_1000x_1() == 2233);
            CHECK(g_reporter->get_trade_result()[1]->buy_price_1000x_1() == 0);
            CHECK(g_reporter->get_trade_result()[1]->buy_price_1000x_2() == 0);
            CHECK(g_reporter->get_trade_result()[1]->buy_price_1000x_3() == 0);
            CHECK(g_reporter->get_trade_result()[1]->sell_quantity_3() == 0);
            CHECK(g_reporter->get_trade_result()[1]->sell_quantity_2() == 0);
            CHECK(g_reporter->get_trade_result()[1]->sell_quantity_1() == 40);
            CHECK(g_reporter->get_trade_result()[1]->buy_quantity_1() == 0);
            CHECK(g_reporter->get_trade_result()[1]->buy_quantity_2() == 0);
            CHECK(g_reporter->get_trade_result()[1]->buy_quantity_3() == 0);

            CHECK(g_reporter->get_trade_result()[2]->symbol() == "600875.SH");
            CHECK(g_reporter->get_trade_result()[2]->price_1000x() == 2233);
            CHECK(g_reporter->get_trade_result()[2]->quantity() == 40);
            CHECK(g_reporter->get_trade_result()[2]->ask_unique_id() == 2);
            CHECK(g_reporter->get_trade_result()[2]->bid_unique_id() == 3);
            CHECK(g_reporter->get_trade_result()[2]->sell_price_1000x_3() == 0);
            CHECK(g_reporter->get_trade_result()[2]->sell_price_1000x_2() == 0);
            CHECK(g_reporter->get_trade_result()[2]->sell_price_1000x_1() == 2233);
            CHECK(g_reporter->get_trade_result()[2]->buy_price_1000x_1() == 0);
            CHECK(g_reporter->get_trade_result()[2]->buy_price_1000x_2() == 0);
            CHECK(g_reporter->get_trade_result()[2]->buy_price_1000x_3() == 0);
            CHECK(g_reporter->get_trade_result()[2]->sell_quantity_3() == 0);
            CHECK(g_reporter->get_trade_result()[2]->sell_quantity_2() == 0);
            CHECK(g_reporter->get_trade_result()[2]->sell_quantity_1() == 40);
            CHECK(g_reporter->get_trade_result()[2]->buy_quantity_1() == 0);
            CHECK(g_reporter->get_trade_result()[2]->buy_quantity_2() == 0);
            CHECK(g_reporter->get_trade_result()[2]->buy_quantity_3() == 0);

            CHECK(g_reporter->get_trade_result()[3]->symbol() == "600875.SH");
            CHECK(g_reporter->get_trade_result()[3]->price_1000x() == 2233);
            CHECK(g_reporter->get_trade_result()[3]->quantity() == 40);
            CHECK(g_reporter->get_trade_result()[3]->ask_unique_id() == 2);
            CHECK(g_reporter->get_trade_result()[3]->bid_unique_id() == 4);
            CHECK(g_reporter->get_trade_result()[3]->sell_price_1000x_3() == 0);
            CHECK(g_reporter->get_trade_result()[3]->sell_price_1000x_2() == 0);
            CHECK(g_reporter->get_trade_result()[3]->sell_price_1000x_1() == 0);
            CHECK(g_reporter->get_trade_result()[3]->buy_price_1000x_1() == 3322);
            CHECK(g_reporter->get_trade_result()[3]->buy_price_1000x_2() == 0);
            CHECK(g_reporter->get_trade_result()[3]->buy_price_1000x_3() == 0);
            CHECK(g_reporter->get_trade_result()[3]->sell_quantity_3() == 0);
            CHECK(g_reporter->get_trade_result()[3]->sell_quantity_2() == 0);
            CHECK(g_reporter->get_trade_result()[3]->sell_quantity_1() == 0);
            CHECK(g_reporter->get_trade_result()[3]->buy_quantity_1() == 60);
            CHECK(g_reporter->get_trade_result()[3]->buy_quantity_2() == 0);
            CHECK(g_reporter->get_trade_result()[3]->buy_quantity_3() == 0);
        }

        SECTION("Sell after buy with step price")
        {
            trade::booker::Booker booker({}, reporter());

            booker.add(TickCreator::order_tick(0, LIMIT, "600875.SH", BUY, 2233, 20));
            booker.add(TickCreator::order_tick(1, LIMIT, "600875.SH", BUY, 3322, 40));
            booker.add(TickCreator::order_tick(2, LIMIT, "600875.SH", BUY, 3333, 80));
            booker.add(TickCreator::order_tick(3, LIMIT, "600875.SH", SELL, 3322, 100));
            booker.add(TickCreator::order_tick(4, LIMIT, "600875.SH", SELL, 3322, 100));

            CHECK(g_reporter->get_trade_result().size() == 3);

            CHECK(g_reporter->get_trade_result()[0]->symbol() == "600875.SH");
            CHECK(g_reporter->get_trade_result()[0]->price_1000x() == 3333);
            CHECK(g_reporter->get_trade_result()[0]->quantity() == 80);
            CHECK(g_reporter->get_trade_result()[0]->ask_unique_id() == 3);
            CHECK(g_reporter->get_trade_result()[0]->bid_unique_id() == 2);
            CHECK(g_reporter->get_trade_result()[0]->sell_price_1000x_3() == 0);
            CHECK(g_reporter->get_trade_result()[0]->sell_price_1000x_2() == 0);
            CHECK(g_reporter->get_trade_result()[0]->sell_price_1000x_1() == 0);
            CHECK(g_reporter->get_trade_result()[0]->buy_price_1000x_1() == 3322);
            CHECK(g_reporter->get_trade_result()[0]->buy_price_1000x_2() == 0);
            CHECK(g_reporter->get_trade_result()[0]->buy_price_1000x_3() == 0);
            CHECK(g_reporter->get_trade_result()[0]->sell_quantity_3() == 0);
            CHECK(g_reporter->get_trade_result()[0]->sell_quantity_2() == 0);
            CHECK(g_reporter->get_trade_result()[0]->sell_quantity_1() == 0);
            CHECK(g_reporter->get_trade_result()[0]->buy_quantity_1() == 40);
            CHECK(g_reporter->get_trade_result()[0]->buy_quantity_2() == 0);
            CHECK(g_reporter->get_trade_result()[0]->buy_quantity_3() == 0);

            CHECK(g_reporter->get_trade_result()[1]->symbol() == "600875.SH");
            CHECK(g_reporter->get_trade_result()[1]->price_1000x() == 3322);
            CHECK(g_reporter->get_trade_result()[1]->quantity() == 20);
            CHECK(g_reporter->get_trade_result()[1]->ask_unique_id() == 3);
            CHECK(g_reporter->get_trade_result()[1]->bid_unique_id() == 1);
            CHECK(g_reporter->get_trade_result()[1]->sell_price_1000x_3() == 0);
            CHECK(g_reporter->get_trade_result()[1]->sell_price_1000x_2() == 0);
            CHECK(g_reporter->get_trade_result()[1]->sell_price_1000x_1() == 0);
            CHECK(g_reporter->get_trade_result()[1]->buy_price_1000x_1() == 3322);
            CHECK(g_reporter->get_trade_result()[1]->buy_price_1000x_2() == 0);
            CHECK(g_reporter->get_trade_result()[1]->buy_price_1000x_3() == 0);
            CHECK(g_reporter->get_trade_result()[1]->sell_quantity_3() == 0);
            CHECK(g_reporter->get_trade_result()[1]->sell_quantity_2() == 0);
            CHECK(g_reporter->get_trade_result()[1]->sell_quantity_1() == 0);
            CHECK(g_reporter->get_trade_result()[1]->buy_quantity_1() == 40);
            CHECK(g_reporter->get_trade_result()[1]->buy_quantity_2() == 0);
            CHECK(g_reporter->get_trade_result()[1]->buy_quantity_3() == 0);

            CHECK(g_reporter->get_trade_result()[2]->symbol() == "600875.SH");
            CHECK(g_reporter->get_trade_result()[2]->price_1000x() == 3322);
            CHECK(g_reporter->get_trade_result()[2]->quantity() == 20);
            CHECK(g_reporter->get_trade_result()[2]->ask_unique_id() == 4);
            CHECK(g_reporter->get_trade_result()[2]->bid_unique_id() == 1);
            CHECK(g_reporter->get_trade_result()[2]->sell_price_1000x_3() == 0);
            CHECK(g_reporter->get_trade_result()[2]->sell_price_1000x_2() == 0);
            CHECK(g_reporter->get_trade_result()[2]->sell_price_1000x_1() == 3322);
            CHECK(g_reporter->get_trade_result()[2]->buy_price_1000x_1() == 2233);
            CHECK(g_reporter->get_trade_result()[2]->buy_price_1000x_2() == 0);
            CHECK(g_reporter->get_trade_result()[2]->buy_price_1000x_3() == 0);
            CHECK(g_reporter->get_trade_result()[2]->sell_quantity_3() == 0);
            CHECK(g_reporter->get_trade_result()[2]->sell_quantity_2() == 0);
            CHECK(g_reporter->get_trade_result()[2]->sell_quantity_1() == 80);
            CHECK(g_reporter->get_trade_result()[2]->buy_quantity_1() == 20);
            CHECK(g_reporter->get_trade_result()[2]->buy_quantity_2() == 0);
            CHECK(g_reporter->get_trade_result()[2]->buy_quantity_3() == 0);
        }

        SECTION("Buy after sell with step price")
        {
            trade::booker::Booker booker({}, reporter());

            booker.add(TickCreator::order_tick(0, LIMIT, "600875.SH", SELL, 2222, 20));
            booker.add(TickCreator::order_tick(1, LIMIT, "600875.SH", SELL, 2233, 40));
            booker.add(TickCreator::order_tick(2, LIMIT, "600875.SH", SELL, 3322, 80));
            booker.add(TickCreator::order_tick(3, LIMIT, "600875.SH", BUY, 2233, 100));
            booker.add(TickCreator::order_tick(4, LIMIT, "600875.SH", BUY, 2233, 100));

            CHECK(g_reporter->get_trade_result().size() == 2);

            CHECK(g_reporter->get_trade_result()[0]->symbol() == "600875.SH");
            CHECK(g_reporter->get_trade_result()[0]->price_1000x() == 2222);
            CHECK(g_reporter->get_trade_result()[0]->quantity() == 20);
            CHECK(g_reporter->get_trade_result()[0]->ask_unique_id() == 0);
            CHECK(g_reporter->get_trade_result()[0]->bid_unique_id() == 3);
            CHECK(g_reporter->get_trade_result()[0]->sell_price_1000x_3() == 0);
            CHECK(g_reporter->get_trade_result()[0]->sell_price_1000x_2() == 0);
            CHECK(g_reporter->get_trade_result()[0]->sell_price_1000x_1() == 3322);
            CHECK(g_reporter->get_trade_result()[0]->buy_price_1000x_1() == 2233);
            CHECK(g_reporter->get_trade_result()[0]->buy_price_1000x_2() == 0);
            CHECK(g_reporter->get_trade_result()[0]->buy_price_1000x_3() == 0);
            CHECK(g_reporter->get_trade_result()[0]->sell_quantity_3() == 0);
            CHECK(g_reporter->get_trade_result()[0]->sell_quantity_2() == 0);
            CHECK(g_reporter->get_trade_result()[0]->sell_quantity_1() == 80);
            CHECK(g_reporter->get_trade_result()[0]->buy_quantity_1() == 40);
            CHECK(g_reporter->get_trade_result()[0]->buy_quantity_2() == 0);
            CHECK(g_reporter->get_trade_result()[0]->buy_quantity_3() == 0);

            CHECK(g_reporter->get_trade_result()[1]->symbol() == "600875.SH");
            CHECK(g_reporter->get_trade_result()[1]->price_1000x() == 2233);
            CHECK(g_reporter->get_trade_result()[1]->quantity() == 40);
            CHECK(g_reporter->get_trade_result()[1]->ask_unique_id() == 1);
            CHECK(g_reporter->get_trade_result()[1]->bid_unique_id() == 3);
            CHECK(g_reporter->get_trade_result()[1]->sell_price_1000x_3() == 0);
            CHECK(g_reporter->get_trade_result()[1]->sell_price_1000x_2() == 0);
            CHECK(g_reporter->get_trade_result()[1]->sell_price_1000x_1() == 3322);
            CHECK(g_reporter->get_trade_result()[1]->buy_price_1000x_1() == 2233);
            CHECK(g_reporter->get_trade_result()[1]->buy_price_1000x_2() == 0);
            CHECK(g_reporter->get_trade_result()[1]->buy_price_1000x_3() == 0);
            CHECK(g_reporter->get_trade_result()[1]->sell_quantity_3() == 0);
            CHECK(g_reporter->get_trade_result()[1]->sell_quantity_2() == 0);
            CHECK(g_reporter->get_trade_result()[1]->sell_quantity_1() == 80);
            CHECK(g_reporter->get_trade_result()[1]->buy_quantity_1() == 40);
            CHECK(g_reporter->get_trade_result()[1]->buy_quantity_2() == 0);
            CHECK(g_reporter->get_trade_result()[1]->buy_quantity_3() == 0);
        }
    }

    SECTION("Limit order with cancel")
    {
        /// Canceling only requires unique_id.

        SECTION("Limit order with cancel before fill")
        {
            trade::booker::Booker booker({}, reporter());

            booker.add(TickCreator::order_tick(0, LIMIT, "600875.SH", BUY, 2233, 50));
            booker.add(TickCreator::order_tick(0, CANCEL, "600875.SH", INV_SIDE, 0, 0));
            booker.add(TickCreator::order_tick(1, LIMIT, "600875.SH", SELL, 2233, 100));

            CHECK(g_reporter->get_trade_result().empty());
        }

        SECTION("Limit order with cancel after partial fill")
        {
            trade::booker::Booker booker({}, reporter());

            booker.add(TickCreator::order_tick(0, LIMIT, "600875.SH", BUY, 2233, 100));
            booker.add(TickCreator::order_tick(1, LIMIT, "600875.SH", SELL, 2233, 50));
            booker.add(TickCreator::order_tick(0, CANCEL, "600875.SH", INV_SIDE, 0, 0));
            booker.add(TickCreator::order_tick(2, LIMIT, "600875.SH", SELL, 2233, 50));

            CHECK(g_reporter->get_trade_result().size() == 1);

            CHECK(g_reporter->get_trade_result()[0]->symbol() == "600875.SH");
            CHECK(g_reporter->get_trade_result()[0]->price_1000x() == 2233);
            CHECK(g_reporter->get_trade_result()[0]->quantity() == 50);
            CHECK(g_reporter->get_trade_result()[0]->ask_unique_id() == 1);
            CHECK(g_reporter->get_trade_result()[0]->bid_unique_id() == 0);
            CHECK(g_reporter->get_trade_result()[0]->sell_price_1000x_3() == 0);
            CHECK(g_reporter->get_trade_result()[0]->sell_price_1000x_2() == 0);
            CHECK(g_reporter->get_trade_result()[0]->sell_price_1000x_1() == 0);
            CHECK(g_reporter->get_trade_result()[0]->buy_price_1000x_1() == 2233);
            CHECK(g_reporter->get_trade_result()[0]->buy_price_1000x_2() == 0);
            CHECK(g_reporter->get_trade_result()[0]->buy_price_1000x_3() == 0);
            CHECK(g_reporter->get_trade_result()[0]->sell_quantity_3() == 0);
            CHECK(g_reporter->get_trade_result()[0]->sell_quantity_2() == 0);
            CHECK(g_reporter->get_trade_result()[0]->sell_quantity_1() == 0);
            CHECK(g_reporter->get_trade_result()[0]->buy_quantity_1() == 50);
            CHECK(g_reporter->get_trade_result()[0]->buy_quantity_2() == 0);
            CHECK(g_reporter->get_trade_result()[0]->buy_quantity_3() == 0);
        }

        SECTION("Limit order with cancel after full fill")
        {
            trade::booker::Booker booker({}, reporter());

            booker.add(TickCreator::order_tick(0, LIMIT, "600875.SH", BUY, 2233, 100));
            booker.add(TickCreator::order_tick(1, LIMIT, "600875.SH", SELL, 2233, 100));
            booker.add(TickCreator::order_tick(0, CANCEL, "600875.SH", INV_SIDE, 0, 0));

            CHECK(g_reporter->get_trade_result().size() == 1);

            CHECK(g_reporter->get_trade_result()[0]->symbol() == "600875.SH");
            CHECK(g_reporter->get_trade_result()[0]->price_1000x() == 2233);
            CHECK(g_reporter->get_trade_result()[0]->quantity() == 100);
            CHECK(g_reporter->get_trade_result()[0]->ask_unique_id() == 1);
            CHECK(g_reporter->get_trade_result()[0]->bid_unique_id() == 0);
            CHECK(g_reporter->get_trade_result()[0]->sell_price_1000x_3() == 0);
            CHECK(g_reporter->get_trade_result()[0]->sell_price_1000x_2() == 0);
            CHECK(g_reporter->get_trade_result()[0]->sell_price_1000x_1() == 0);
            CHECK(g_reporter->get_trade_result()[0]->buy_price_1000x_1() == 0);
            CHECK(g_reporter->get_trade_result()[0]->buy_price_1000x_2() == 0);
            CHECK(g_reporter->get_trade_result()[0]->buy_price_1000x_3() == 0);
            CHECK(g_reporter->get_trade_result()[0]->sell_quantity_3() == 0);
            CHECK(g_reporter->get_trade_result()[0]->sell_quantity_2() == 0);
            CHECK(g_reporter->get_trade_result()[0]->sell_quantity_1() == 0);
            CHECK(g_reporter->get_trade_result()[0]->buy_quantity_1() == 0);
            CHECK(g_reporter->get_trade_result()[0]->buy_quantity_2() == 0);
            CHECK(g_reporter->get_trade_result()[0]->buy_quantity_3() == 0);
        }
    }

    SECTION("Market order matching with 1:1 matching")
    {
        SECTION("Sell after buy with market price")
        {
            trade::booker::Booker booker({}, reporter());

            booker.add(TickCreator::order_tick(0, LIMIT, "600875.SH", BUY, 2233, 100));
            booker.add(TickCreator::order_tick(1, LIMIT, "600875.SH", BUY, 3322, 100));
            booker.add(TickCreator::order_tick(2, MARKET, "600875.SH", SELL, 0, 100));
            booker.trade(TickCreator::trade_tick(2, 1, "600875.SH", 3322, 100, 100000000));

            CHECK(g_reporter->get_trade_result().size() == 1);

            CHECK(g_reporter->get_trade_result()[0]->symbol() == "600875.SH");
            CHECK(g_reporter->get_trade_result()[0]->price_1000x() == 3322);
            CHECK(g_reporter->get_trade_result()[0]->quantity() == 100);
            CHECK(g_reporter->get_trade_result()[0]->ask_unique_id() == 2);
            CHECK(g_reporter->get_trade_result()[0]->bid_unique_id() == 1);
            CHECK(g_reporter->get_trade_result()[0]->sell_price_1000x_3() == 0);
            CHECK(g_reporter->get_trade_result()[0]->sell_price_1000x_2() == 0);
            CHECK(g_reporter->get_trade_result()[0]->sell_price_1000x_1() == 0);
            CHECK(g_reporter->get_trade_result()[0]->buy_price_1000x_1() == 2233);
            CHECK(g_reporter->get_trade_result()[0]->buy_price_1000x_2() == 0);
            CHECK(g_reporter->get_trade_result()[0]->buy_price_1000x_3() == 0);
            CHECK(g_reporter->get_trade_result()[0]->sell_quantity_3() == 0);
            CHECK(g_reporter->get_trade_result()[0]->sell_quantity_2() == 0);
            CHECK(g_reporter->get_trade_result()[0]->sell_quantity_1() == 0);
            CHECK(g_reporter->get_trade_result()[0]->buy_quantity_1() == 100);
            CHECK(g_reporter->get_trade_result()[0]->buy_quantity_2() == 0);
            CHECK(g_reporter->get_trade_result()[0]->buy_quantity_3() == 0);
        }

        SECTION("Buy after sell with market price")
        {
            trade::booker::Booker booker({}, reporter());

            booker.add(TickCreator::order_tick(0, LIMIT, "600875.SH", SELL, 2233, 100));
            booker.add(TickCreator::order_tick(1, LIMIT, "600875.SH", SELL, 3322, 100));
            booker.add(TickCreator::order_tick(2, MARKET, "600875.SH", BUY, 0, 100));
            booker.trade(TickCreator::trade_tick(0, 2, "600875.SH", 2233, 100, 100000000));

            CHECK(g_reporter->get_trade_result().size() == 1);

            CHECK(g_reporter->get_trade_result()[0]->symbol() == "600875.SH");
            CHECK(g_reporter->get_trade_result()[0]->price_1000x() == 2233);
            CHECK(g_reporter->get_trade_result()[0]->quantity() == 100);
            CHECK(g_reporter->get_trade_result()[0]->ask_unique_id() == 0);
            CHECK(g_reporter->get_trade_result()[0]->bid_unique_id() == 2);
            CHECK(g_reporter->get_trade_result()[0]->sell_price_1000x_3() == 0);
            CHECK(g_reporter->get_trade_result()[0]->sell_price_1000x_2() == 0);
            CHECK(g_reporter->get_trade_result()[0]->sell_price_1000x_1() == 3322);
            CHECK(g_reporter->get_trade_result()[0]->buy_price_1000x_1() == 0);
            CHECK(g_reporter->get_trade_result()[0]->buy_price_1000x_2() == 0);
            CHECK(g_reporter->get_trade_result()[0]->buy_price_1000x_3() == 0);
            CHECK(g_reporter->get_trade_result()[0]->sell_quantity_3() == 0);
            CHECK(g_reporter->get_trade_result()[0]->sell_quantity_2() == 0);
            CHECK(g_reporter->get_trade_result()[0]->sell_quantity_1() == 100);
            CHECK(g_reporter->get_trade_result()[0]->buy_quantity_1() == 0);
            CHECK(g_reporter->get_trade_result()[0]->buy_quantity_2() == 0);
            CHECK(g_reporter->get_trade_result()[0]->buy_quantity_3() == 0);
        }
    }

    SECTION("Market order matching with M:N matching")
    {
        SECTION("Buy after sell with market price")
        {
            trade::booker::Booker booker({}, reporter());

            booker.add(TickCreator::order_tick(0, LIMIT, "600875.SH", SELL, 2233, 20));
            booker.add(TickCreator::order_tick(1, LIMIT, "600875.SH", SELL, 2233, 40));
            booker.add(TickCreator::order_tick(2, LIMIT, "600875.SH", SELL, 2233, 80));
            booker.add(TickCreator::order_tick(3, MARKET, "600875.SH", BUY, 0, 100));
            booker.trade(TickCreator::trade_tick(0, 3, "600875.SH", 2233, 20, 100000000));
            booker.trade(TickCreator::trade_tick(1, 3, "600875.SH", 2233, 40, 100000000));
            booker.trade(TickCreator::trade_tick(2, 3, "600875.SH", 2233, 40, 100000000));
            booker.add(TickCreator::order_tick(4, MARKET, "600875.SH", BUY, 0, 100));
            booker.trade(TickCreator::trade_tick(1, 4, "600875.SH", 2233, 40, 100000000));

            CHECK(g_reporter->get_trade_result().size() == 4);

            CHECK(g_reporter->get_trade_result()[0]->symbol() == "600875.SH");
            CHECK(g_reporter->get_trade_result()[0]->price_1000x() == 2233);
            CHECK(g_reporter->get_trade_result()[0]->quantity() == 20);
            CHECK(g_reporter->get_trade_result()[0]->ask_unique_id() == 0);
            CHECK(g_reporter->get_trade_result()[0]->bid_unique_id() == 3);
            CHECK(g_reporter->get_trade_result()[0]->sell_price_1000x_3() == 0);
            CHECK(g_reporter->get_trade_result()[0]->sell_price_1000x_2() == 0);
            CHECK(g_reporter->get_trade_result()[0]->sell_price_1000x_1() == 2233);
            CHECK(g_reporter->get_trade_result()[0]->buy_price_1000x_1() == 0);
            CHECK(g_reporter->get_trade_result()[0]->buy_price_1000x_2() == 0);
            CHECK(g_reporter->get_trade_result()[0]->buy_price_1000x_3() == 0);
            CHECK(g_reporter->get_trade_result()[0]->sell_quantity_3() == 0);
            CHECK(g_reporter->get_trade_result()[0]->sell_quantity_2() == 0);
            CHECK(g_reporter->get_trade_result()[0]->sell_quantity_1() == 120);
            CHECK(g_reporter->get_trade_result()[0]->buy_quantity_1() == 0);
            CHECK(g_reporter->get_trade_result()[0]->buy_quantity_2() == 0);
            CHECK(g_reporter->get_trade_result()[0]->buy_quantity_3() == 0);

            CHECK(g_reporter->get_trade_result()[1]->symbol() == "600875.SH");
            CHECK(g_reporter->get_trade_result()[1]->price_1000x() == 2233);
            CHECK(g_reporter->get_trade_result()[1]->quantity() == 40);
            CHECK(g_reporter->get_trade_result()[1]->ask_unique_id() == 1);
            CHECK(g_reporter->get_trade_result()[1]->bid_unique_id() == 3);
            CHECK(g_reporter->get_trade_result()[1]->sell_price_1000x_3() == 0);
            CHECK(g_reporter->get_trade_result()[1]->sell_price_1000x_2() == 0);
            CHECK(g_reporter->get_trade_result()[1]->sell_price_1000x_1() == 2233);
            CHECK(g_reporter->get_trade_result()[1]->buy_price_1000x_1() == 0);
            CHECK(g_reporter->get_trade_result()[1]->buy_price_1000x_2() == 0);
            CHECK(g_reporter->get_trade_result()[1]->buy_price_1000x_3() == 0);
            CHECK(g_reporter->get_trade_result()[1]->sell_quantity_3() == 0);
            CHECK(g_reporter->get_trade_result()[1]->sell_quantity_2() == 0);
            CHECK(g_reporter->get_trade_result()[1]->sell_quantity_1() == 80);
            CHECK(g_reporter->get_trade_result()[1]->buy_quantity_1() == 0);
            CHECK(g_reporter->get_trade_result()[1]->buy_quantity_2() == 0);
            CHECK(g_reporter->get_trade_result()[1]->buy_quantity_3() == 0);

            CHECK(g_reporter->get_trade_result()[2]->symbol() == "600875.SH");
            CHECK(g_reporter->get_trade_result()[2]->price_1000x() == 2233);
            CHECK(g_reporter->get_trade_result()[2]->quantity() == 40);
            CHECK(g_reporter->get_trade_result()[2]->ask_unique_id() == 2);
            CHECK(g_reporter->get_trade_result()[2]->bid_unique_id() == 3);
            CHECK(g_reporter->get_trade_result()[2]->sell_price_1000x_3() == 0);
            CHECK(g_reporter->get_trade_result()[2]->sell_price_1000x_2() == 0);
            CHECK(g_reporter->get_trade_result()[2]->sell_price_1000x_1() == 2233);
            CHECK(g_reporter->get_trade_result()[2]->buy_price_1000x_1() == 0);
            CHECK(g_reporter->get_trade_result()[2]->buy_price_1000x_2() == 0);
            CHECK(g_reporter->get_trade_result()[2]->buy_price_1000x_3() == 0);
            CHECK(g_reporter->get_trade_result()[2]->sell_quantity_3() == 0);
            CHECK(g_reporter->get_trade_result()[2]->sell_quantity_2() == 0);
            CHECK(g_reporter->get_trade_result()[2]->sell_quantity_1() == 40);
            CHECK(g_reporter->get_trade_result()[2]->buy_quantity_1() == 0);
            CHECK(g_reporter->get_trade_result()[2]->buy_quantity_2() == 0);
            CHECK(g_reporter->get_trade_result()[2]->buy_quantity_3() == 0);

            CHECK(g_reporter->get_trade_result()[3]->symbol() == "600875.SH");
            CHECK(g_reporter->get_trade_result()[3]->price_1000x() == 2233);
            CHECK(g_reporter->get_trade_result()[3]->quantity() == 40);
            CHECK(g_reporter->get_trade_result()[3]->ask_unique_id() == 2);
            CHECK(g_reporter->get_trade_result()[3]->bid_unique_id() == 4);
            CHECK(g_reporter->get_trade_result()[3]->sell_price_1000x_3() == 0);
            CHECK(g_reporter->get_trade_result()[3]->sell_price_1000x_2() == 0);
            CHECK(g_reporter->get_trade_result()[3]->sell_price_1000x_1() == 0);
            CHECK(g_reporter->get_trade_result()[3]->buy_price_1000x_1() == 0);
            CHECK(g_reporter->get_trade_result()[3]->buy_price_1000x_2() == 0);
            CHECK(g_reporter->get_trade_result()[3]->buy_price_1000x_3() == 0);
            CHECK(g_reporter->get_trade_result()[3]->sell_quantity_3() == 0);
            CHECK(g_reporter->get_trade_result()[3]->sell_quantity_2() == 0);
            CHECK(g_reporter->get_trade_result()[3]->sell_quantity_1() == 0);
            CHECK(g_reporter->get_trade_result()[3]->buy_quantity_1() == 0);
            CHECK(g_reporter->get_trade_result()[3]->buy_quantity_2() == 0);
            CHECK(g_reporter->get_trade_result()[3]->buy_quantity_3() == 0);
        }

        SECTION("Buy after sell with market price")
        {
            trade::booker::Booker booker({}, reporter());

            booker.add(TickCreator::order_tick(0, LIMIT, "600875.SH", SELL, 2233, 20));
            booker.add(TickCreator::order_tick(1, LIMIT, "600875.SH", SELL, 2233, 40));
            booker.add(TickCreator::order_tick(2, LIMIT, "600875.SH", SELL, 2233, 80));
            booker.add(TickCreator::order_tick(3, MARKET, "600875.SH", BUY, 0, 100));
            booker.trade(TickCreator::trade_tick(0, 3, "600875.SH", 2233, 20, 100000000));
            booker.trade(TickCreator::trade_tick(1, 3, "600875.SH", 2233, 40, 100000000));
            booker.trade(TickCreator::trade_tick(2, 3, "600875.SH", 2233, 40, 100000000));
            booker.add(TickCreator::order_tick(4, MARKET, "600875.SH", BUY, 0, 100));
            booker.trade(TickCreator::trade_tick(2, 4, "600875.SH", 2233, 40, 100000000));

            CHECK(g_reporter->get_trade_result().size() == 4);

            CHECK(g_reporter->get_trade_result()[0]->symbol() == "600875.SH");
            CHECK(g_reporter->get_trade_result()[0]->price_1000x() == 2233);
            CHECK(g_reporter->get_trade_result()[0]->quantity() == 20);
            CHECK(g_reporter->get_trade_result()[0]->ask_unique_id() == 0);
            CHECK(g_reporter->get_trade_result()[0]->bid_unique_id() == 3);
            CHECK(g_reporter->get_trade_result()[0]->sell_price_1000x_3() == 0);
            CHECK(g_reporter->get_trade_result()[0]->sell_price_1000x_2() == 0);
            CHECK(g_reporter->get_trade_result()[0]->sell_price_1000x_1() == 2233);
            CHECK(g_reporter->get_trade_result()[0]->buy_price_1000x_1() == 0);
            CHECK(g_reporter->get_trade_result()[0]->buy_price_1000x_2() == 0);
            CHECK(g_reporter->get_trade_result()[0]->buy_price_1000x_3() == 0);
            CHECK(g_reporter->get_trade_result()[0]->sell_quantity_3() == 0);
            CHECK(g_reporter->get_trade_result()[0]->sell_quantity_2() == 0);
            CHECK(g_reporter->get_trade_result()[0]->sell_quantity_1() == 120);
            CHECK(g_reporter->get_trade_result()[0]->buy_quantity_1() == 0);
            CHECK(g_reporter->get_trade_result()[0]->buy_quantity_2() == 0);
            CHECK(g_reporter->get_trade_result()[0]->buy_quantity_3() == 0);

            CHECK(g_reporter->get_trade_result()[1]->symbol() == "600875.SH");
            CHECK(g_reporter->get_trade_result()[1]->price_1000x() == 2233);
            CHECK(g_reporter->get_trade_result()[1]->quantity() == 40);
            CHECK(g_reporter->get_trade_result()[1]->ask_unique_id() == 1);
            CHECK(g_reporter->get_trade_result()[1]->bid_unique_id() == 3);
            CHECK(g_reporter->get_trade_result()[1]->sell_price_1000x_3() == 0);
            CHECK(g_reporter->get_trade_result()[1]->sell_price_1000x_2() == 0);
            CHECK(g_reporter->get_trade_result()[1]->sell_price_1000x_1() == 2233);
            CHECK(g_reporter->get_trade_result()[1]->buy_price_1000x_1() == 0);
            CHECK(g_reporter->get_trade_result()[1]->buy_price_1000x_2() == 0);
            CHECK(g_reporter->get_trade_result()[1]->buy_price_1000x_3() == 0);
            CHECK(g_reporter->get_trade_result()[1]->sell_quantity_3() == 0);
            CHECK(g_reporter->get_trade_result()[1]->sell_quantity_2() == 0);
            CHECK(g_reporter->get_trade_result()[1]->sell_quantity_1() == 80);
            CHECK(g_reporter->get_trade_result()[1]->buy_quantity_1() == 0);
            CHECK(g_reporter->get_trade_result()[1]->buy_quantity_2() == 0);
            CHECK(g_reporter->get_trade_result()[1]->buy_quantity_3() == 0);

            CHECK(g_reporter->get_trade_result()[2]->symbol() == "600875.SH");
            CHECK(g_reporter->get_trade_result()[2]->price_1000x() == 2233);
            CHECK(g_reporter->get_trade_result()[2]->quantity() == 40);
            CHECK(g_reporter->get_trade_result()[2]->ask_unique_id() == 2);
            CHECK(g_reporter->get_trade_result()[2]->bid_unique_id() == 3);
            CHECK(g_reporter->get_trade_result()[2]->sell_price_1000x_3() == 0);
            CHECK(g_reporter->get_trade_result()[2]->sell_price_1000x_2() == 0);
            CHECK(g_reporter->get_trade_result()[2]->sell_price_1000x_1() == 2233);
            CHECK(g_reporter->get_trade_result()[2]->buy_price_1000x_1() == 0);
            CHECK(g_reporter->get_trade_result()[2]->buy_price_1000x_2() == 0);
            CHECK(g_reporter->get_trade_result()[2]->buy_price_1000x_3() == 0);
            CHECK(g_reporter->get_trade_result()[2]->sell_quantity_3() == 0);
            CHECK(g_reporter->get_trade_result()[2]->sell_quantity_2() == 0);
            CHECK(g_reporter->get_trade_result()[2]->sell_quantity_1() == 40);
            CHECK(g_reporter->get_trade_result()[2]->buy_quantity_1() == 0);
            CHECK(g_reporter->get_trade_result()[2]->buy_quantity_2() == 0);
            CHECK(g_reporter->get_trade_result()[2]->buy_quantity_3() == 0);

            CHECK(g_reporter->get_trade_result()[3]->symbol() == "600875.SH");
            CHECK(g_reporter->get_trade_result()[3]->price_1000x() == 2233);
            CHECK(g_reporter->get_trade_result()[3]->quantity() == 40);
            CHECK(g_reporter->get_trade_result()[3]->ask_unique_id() == 2);
            CHECK(g_reporter->get_trade_result()[3]->bid_unique_id() == 4);
            CHECK(g_reporter->get_trade_result()[3]->sell_price_1000x_3() == 0);
            CHECK(g_reporter->get_trade_result()[3]->sell_price_1000x_2() == 0);
            CHECK(g_reporter->get_trade_result()[3]->sell_price_1000x_1() == 0);
            CHECK(g_reporter->get_trade_result()[3]->buy_price_1000x_1() == 0);
            CHECK(g_reporter->get_trade_result()[3]->buy_price_1000x_2() == 0);
            CHECK(g_reporter->get_trade_result()[3]->buy_price_1000x_3() == 0);
            CHECK(g_reporter->get_trade_result()[3]->sell_quantity_3() == 0);
            CHECK(g_reporter->get_trade_result()[3]->sell_quantity_2() == 0);
            CHECK(g_reporter->get_trade_result()[3]->sell_quantity_1() == 0);
            CHECK(g_reporter->get_trade_result()[3]->buy_quantity_1() == 0);
            CHECK(g_reporter->get_trade_result()[3]->buy_quantity_2() == 0);
            CHECK(g_reporter->get_trade_result()[3]->buy_quantity_3() == 0);
        }

        SECTION("Sell after buy with step market price")
        {
            trade::booker::Booker booker({}, reporter());

            booker.add(TickCreator::order_tick(0, LIMIT, "600875.SH", BUY, 2233, 20));
            booker.add(TickCreator::order_tick(1, LIMIT, "600875.SH", BUY, 3322, 40));
            booker.add(TickCreator::order_tick(2, LIMIT, "600875.SH", BUY, 3333, 80));
            booker.add(TickCreator::order_tick(3, MARKET, "600875.SH", SELL, 0, 100));
            booker.add(TickCreator::order_tick(4, MARKET, "600875.SH", SELL, 0, 100));

            CHECK(g_reporter->get_trade_result().size() == 2);

            CHECK(g_reporter->get_trade_result()[0]->symbol() == "600875.SH");
            CHECK(g_reporter->get_trade_result()[0]->price_1000x() == 3333);
            CHECK(g_reporter->get_trade_result()[0]->quantity() == 80);
            CHECK(g_reporter->get_trade_result()[0]->ask_unique_id() == 3);
            CHECK(g_reporter->get_trade_result()[0]->bid_unique_id() == 2);
            CHECK(g_reporter->get_trade_result()[0]->sell_price_1000x_3() == 0);
            CHECK(g_reporter->get_trade_result()[0]->sell_price_1000x_2() == 0);
            CHECK(g_reporter->get_trade_result()[0]->sell_price_1000x_1() == 0);
            CHECK(g_reporter->get_trade_result()[0]->buy_price_1000x_1() == 3322);
            CHECK(g_reporter->get_trade_result()[0]->buy_price_1000x_2() == 0);
            CHECK(g_reporter->get_trade_result()[0]->buy_price_1000x_3() == 0);
            CHECK(g_reporter->get_trade_result()[0]->sell_quantity_3() == 0);
            CHECK(g_reporter->get_trade_result()[0]->sell_quantity_2() == 0);
            CHECK(g_reporter->get_trade_result()[0]->sell_quantity_1() == 0);
            CHECK(g_reporter->get_trade_result()[0]->buy_quantity_1() == 40);
            CHECK(g_reporter->get_trade_result()[0]->buy_quantity_2() == 0);
            CHECK(g_reporter->get_trade_result()[0]->buy_quantity_3() == 0);

            CHECK(g_reporter->get_trade_result()[1]->symbol() == "600875.SH");
            CHECK(g_reporter->get_trade_result()[1]->price_1000x() == 3322);
            CHECK(g_reporter->get_trade_result()[1]->quantity() == 20);
            CHECK(g_reporter->get_trade_result()[1]->ask_unique_id() == 3);
            CHECK(g_reporter->get_trade_result()[1]->bid_unique_id() == 1);
            CHECK(g_reporter->get_trade_result()[1]->sell_price_1000x_3() == 0);
            CHECK(g_reporter->get_trade_result()[1]->sell_price_1000x_2() == 0);
            CHECK(g_reporter->get_trade_result()[1]->sell_price_1000x_1() == 0);
            CHECK(g_reporter->get_trade_result()[1]->buy_price_1000x_1() == 3322);
            CHECK(g_reporter->get_trade_result()[1]->buy_price_1000x_2() == 0);
            CHECK(g_reporter->get_trade_result()[1]->buy_price_1000x_3() == 0);
            CHECK(g_reporter->get_trade_result()[1]->sell_quantity_3() == 0);
            CHECK(g_reporter->get_trade_result()[1]->sell_quantity_2() == 0);
            CHECK(g_reporter->get_trade_result()[1]->sell_quantity_1() == 0);
            CHECK(g_reporter->get_trade_result()[1]->buy_quantity_1() == 40);
            CHECK(g_reporter->get_trade_result()[1]->buy_quantity_2() == 0);
            CHECK(g_reporter->get_trade_result()[1]->buy_quantity_3() == 0);
        }

        SECTION("Buy after sell with step market price")
        {
            trade::booker::Booker booker({}, reporter());

            booker.add(TickCreator::order_tick(0, LIMIT, "600875.SH", SELL, 2222, 20));
            booker.add(TickCreator::order_tick(1, LIMIT, "600875.SH", SELL, 2233, 40));
            booker.add(TickCreator::order_tick(2, LIMIT, "600875.SH", SELL, 3322, 80));
            booker.add(TickCreator::order_tick(3, MARKET, "600875.SH", BUY, 0, 100));
            booker.trade(TickCreator::trade_tick(0, 3, "600875.SH", 2222, 20, 100000000));
            booker.add(TickCreator::order_tick(4, MARKET, "600875.SH", BUY, 0, 100));
            booker.trade(TickCreator::trade_tick(1, 4, "600875.SH", 2233, 40, 100000000));

            CHECK(g_reporter->get_trade_result().size() == 2);

            CHECK(g_reporter->get_trade_result()[0]->symbol() == "600875.SH");
            CHECK(g_reporter->get_trade_result()[0]->price_1000x() == 2222);
            CHECK(g_reporter->get_trade_result()[0]->quantity() == 20);
            CHECK(g_reporter->get_trade_result()[0]->ask_unique_id() == 0);
            CHECK(g_reporter->get_trade_result()[0]->bid_unique_id() == 3);
            CHECK(g_reporter->get_trade_result()[0]->sell_price_1000x_3() == 0);
            CHECK(g_reporter->get_trade_result()[0]->sell_price_1000x_2() == 3322);
            CHECK(g_reporter->get_trade_result()[0]->sell_price_1000x_1() == 2233);
            CHECK(g_reporter->get_trade_result()[0]->buy_price_1000x_1() == 0);
            CHECK(g_reporter->get_trade_result()[0]->buy_price_1000x_2() == 0);
            CHECK(g_reporter->get_trade_result()[0]->buy_price_1000x_3() == 0);
            CHECK(g_reporter->get_trade_result()[0]->sell_quantity_3() == 0);
            CHECK(g_reporter->get_trade_result()[0]->sell_quantity_2() == 80);
            CHECK(g_reporter->get_trade_result()[0]->sell_quantity_1() == 40);
            CHECK(g_reporter->get_trade_result()[0]->buy_quantity_1() == 0);
            CHECK(g_reporter->get_trade_result()[0]->buy_quantity_2() == 0);
            CHECK(g_reporter->get_trade_result()[0]->buy_quantity_3() == 0);

            CHECK(g_reporter->get_trade_result()[1]->symbol() == "600875.SH");
            CHECK(g_reporter->get_trade_result()[1]->price_1000x() == 2233);
            CHECK(g_reporter->get_trade_result()[1]->quantity() == 40);
            CHECK(g_reporter->get_trade_result()[1]->ask_unique_id() == 1);
            CHECK(g_reporter->get_trade_result()[1]->bid_unique_id() == 4);
            CHECK(g_reporter->get_trade_result()[1]->sell_price_1000x_3() == 0);
            CHECK(g_reporter->get_trade_result()[1]->sell_price_1000x_2() == 0);
            CHECK(g_reporter->get_trade_result()[1]->sell_price_1000x_1() == 3322);
            CHECK(g_reporter->get_trade_result()[1]->buy_price_1000x_1() == 2222);
            CHECK(g_reporter->get_trade_result()[1]->buy_price_1000x_2() == 0);
            CHECK(g_reporter->get_trade_result()[1]->buy_price_1000x_3() == 0);
            CHECK(g_reporter->get_trade_result()[1]->sell_quantity_3() == 0);
            CHECK(g_reporter->get_trade_result()[1]->sell_quantity_2() == 0);
            CHECK(g_reporter->get_trade_result()[1]->sell_quantity_1() == 80);
            CHECK(g_reporter->get_trade_result()[1]->buy_quantity_1() == 80);
            CHECK(g_reporter->get_trade_result()[1]->buy_quantity_2() == 0);
            CHECK(g_reporter->get_trade_result()[1]->buy_quantity_3() == 0);
        }
    }

    SECTION("Best price order matching")
    {
        SECTION("Sell after buy with best price")
        {
            trade::booker::Booker booker({}, reporter());

            booker.add(TickCreator::order_tick(0, LIMIT, "600875.SH", BUY, 2233, 20));
            booker.add(TickCreator::order_tick(1, BEST_PRICE, "600875.SH", BUY, 0, 40));
            booker.add(TickCreator::order_tick(2, LIMIT, "600875.SH", SELL, 2233, 100));

            CHECK(g_reporter->get_trade_result().size() == 2);

            CHECK(g_reporter->get_trade_result()[0]->symbol() == "600875.SH");
            CHECK(g_reporter->get_trade_result()[0]->price_1000x() == 2233);
            CHECK(g_reporter->get_trade_result()[0]->quantity() == 20);
            CHECK(g_reporter->get_trade_result()[0]->ask_unique_id() == 2);
            CHECK(g_reporter->get_trade_result()[0]->bid_unique_id() == 0);
            CHECK(g_reporter->get_trade_result()[0]->sell_price_1000x_3() == 0);
            CHECK(g_reporter->get_trade_result()[0]->sell_price_1000x_2() == 0);
            CHECK(g_reporter->get_trade_result()[0]->sell_price_1000x_1() == 2233);
            CHECK(g_reporter->get_trade_result()[0]->buy_price_1000x_1() == 0);
            CHECK(g_reporter->get_trade_result()[0]->buy_price_1000x_2() == 0);
            CHECK(g_reporter->get_trade_result()[0]->buy_price_1000x_3() == 0);
            CHECK(g_reporter->get_trade_result()[0]->sell_quantity_3() == 0);
            CHECK(g_reporter->get_trade_result()[0]->sell_quantity_2() == 0);
            CHECK(g_reporter->get_trade_result()[0]->sell_quantity_1() == 40);
            CHECK(g_reporter->get_trade_result()[0]->buy_quantity_1() == 0);
            CHECK(g_reporter->get_trade_result()[0]->buy_quantity_2() == 0);
            CHECK(g_reporter->get_trade_result()[0]->buy_quantity_3() == 0);

            CHECK(g_reporter->get_trade_result()[1]->symbol() == "600875.SH");
            CHECK(g_reporter->get_trade_result()[1]->price_1000x() == 2233);
            CHECK(g_reporter->get_trade_result()[1]->quantity() == 40);
            CHECK(g_reporter->get_trade_result()[1]->ask_unique_id() == 2);
            CHECK(g_reporter->get_trade_result()[1]->bid_unique_id() == 1);
            CHECK(g_reporter->get_trade_result()[1]->sell_price_1000x_3() == 0);
            CHECK(g_reporter->get_trade_result()[1]->sell_price_1000x_2() == 0);
            CHECK(g_reporter->get_trade_result()[1]->sell_price_1000x_1() == 2233);
            CHECK(g_reporter->get_trade_result()[1]->buy_price_1000x_1() == 0);
            CHECK(g_reporter->get_trade_result()[1]->buy_price_1000x_2() == 0);
            CHECK(g_reporter->get_trade_result()[1]->buy_price_1000x_3() == 0);
            CHECK(g_reporter->get_trade_result()[1]->sell_quantity_3() == 0);
            CHECK(g_reporter->get_trade_result()[1]->sell_quantity_2() == 0);
            CHECK(g_reporter->get_trade_result()[1]->sell_quantity_1() == 40);
            CHECK(g_reporter->get_trade_result()[1]->buy_quantity_1() == 0);
            CHECK(g_reporter->get_trade_result()[1]->buy_quantity_2() == 0);
            CHECK(g_reporter->get_trade_result()[1]->buy_quantity_3() == 0);
        }

        SECTION("Buy after sell with best price")
        {
            trade::booker::Booker booker({}, reporter());

            booker.add(TickCreator::order_tick(0, LIMIT, "600875.SH", SELL, 2233, 20));
            booker.add(TickCreator::order_tick(1, BEST_PRICE, "600875.SH", SELL, 0, 40));
            booker.add(TickCreator::order_tick(2, LIMIT, "600875.SH", BUY, 2233, 100));

            CHECK(g_reporter->get_trade_result().size() == 2);

            CHECK(g_reporter->get_trade_result()[0]->symbol() == "600875.SH");
            CHECK(g_reporter->get_trade_result()[0]->price_1000x() == 2233);
            CHECK(g_reporter->get_trade_result()[0]->quantity() == 20);
            CHECK(g_reporter->get_trade_result()[0]->ask_unique_id() == 0);
            CHECK(g_reporter->get_trade_result()[0]->bid_unique_id() == 2);
            CHECK(g_reporter->get_trade_result()[0]->sell_price_1000x_3() == 0);
            CHECK(g_reporter->get_trade_result()[0]->sell_price_1000x_2() == 0);
            CHECK(g_reporter->get_trade_result()[0]->sell_price_1000x_1() == 0);
            CHECK(g_reporter->get_trade_result()[0]->buy_price_1000x_1() == 2233);
            CHECK(g_reporter->get_trade_result()[0]->buy_price_1000x_2() == 0);
            CHECK(g_reporter->get_trade_result()[0]->buy_price_1000x_3() == 0);
            CHECK(g_reporter->get_trade_result()[0]->sell_quantity_3() == 0);
            CHECK(g_reporter->get_trade_result()[0]->sell_quantity_2() == 0);
            CHECK(g_reporter->get_trade_result()[0]->sell_quantity_1() == 0);
            CHECK(g_reporter->get_trade_result()[0]->buy_quantity_1() == 40);
            CHECK(g_reporter->get_trade_result()[0]->buy_quantity_2() == 0);
            CHECK(g_reporter->get_trade_result()[0]->buy_quantity_3() == 0);

            CHECK(g_reporter->get_trade_result()[1]->symbol() == "600875.SH");
            CHECK(g_reporter->get_trade_result()[1]->price_1000x() == 2233);
            CHECK(g_reporter->get_trade_result()[1]->quantity() == 40);
            CHECK(g_reporter->get_trade_result()[1]->ask_unique_id() == 1);
            CHECK(g_reporter->get_trade_result()[1]->bid_unique_id() == 2);
            CHECK(g_reporter->get_trade_result()[1]->sell_price_1000x_3() == 0);
            CHECK(g_reporter->get_trade_result()[1]->sell_price_1000x_2() == 0);
            CHECK(g_reporter->get_trade_result()[1]->sell_price_1000x_1() == 0);
            CHECK(g_reporter->get_trade_result()[1]->buy_price_1000x_1() == 2233);
            CHECK(g_reporter->get_trade_result()[1]->buy_price_1000x_2() == 0);
            CHECK(g_reporter->get_trade_result()[1]->buy_price_1000x_3() == 0);
            CHECK(g_reporter->get_trade_result()[1]->sell_quantity_3() == 0);
            CHECK(g_reporter->get_trade_result()[1]->sell_quantity_2() == 0);
            CHECK(g_reporter->get_trade_result()[1]->sell_quantity_1() == 0);
            CHECK(g_reporter->get_trade_result()[1]->buy_quantity_1() == 40);
            CHECK(g_reporter->get_trade_result()[1]->buy_quantity_2() == 0);
            CHECK(g_reporter->get_trade_result()[1]->buy_quantity_3() == 0);
        }
    }

    SECTION("Call auction stage")
    {
        SECTION("No trade after call auction")
        {
            trade::booker::Booker booker({}, reporter());

            booker.add(TickCreator::order_tick(0, LIMIT, "600875.SH", SELL, 2233, 20, 91500000));
            booker.add(TickCreator::order_tick(1, LIMIT, "600875.SH", SELL, 3322, 40, 91500000));
            booker.add(TickCreator::order_tick(2, LIMIT, "600875.SH", SELL, 3333, 80, 91500000));
            booker.add(TickCreator::order_tick(3, LIMIT, "600875.SH", BUY, 2222, 100, 91500000));
            booker.add(TickCreator::order_tick(4, LIMIT, "600875.SH", BUY, 2233, 100, 91500000));
            booker.add(TickCreator::order_tick(5, LIMIT, "600875.SH", BUY, 3322, 100, 91500000));

            booker.trade(TickCreator::trade_tick(1, 5, "600875.SH", 3322, 40, 92500000));
            booker.trade(TickCreator::trade_tick(0, 5, "600875.SH", 3322, 20, 92500000));

            booker.switch_to_continuous_stage();

            CHECK(g_reporter->get_trade_result().size() == 2);

            CHECK(g_reporter->get_trade_result()[0]->symbol() == "600875.SH");
            CHECK(g_reporter->get_trade_result()[0]->price_1000x() == 3322);
            CHECK(g_reporter->get_trade_result()[0]->quantity() == 40);
            CHECK(g_reporter->get_trade_result()[0]->ask_unique_id() == 1);
            CHECK(g_reporter->get_trade_result()[0]->bid_unique_id() == 5);
            CHECK(g_reporter->get_trade_result()[0]->sell_price_1000x_3() == 0);
            CHECK(g_reporter->get_trade_result()[0]->sell_price_1000x_2() == 0);
            CHECK(g_reporter->get_trade_result()[0]->sell_price_1000x_1() == 0);
            CHECK(g_reporter->get_trade_result()[0]->buy_price_1000x_1() == 0);
            CHECK(g_reporter->get_trade_result()[0]->buy_price_1000x_2() == 0);
            CHECK(g_reporter->get_trade_result()[0]->buy_price_1000x_3() == 0);
            CHECK(g_reporter->get_trade_result()[0]->sell_quantity_3() == 0);
            CHECK(g_reporter->get_trade_result()[0]->sell_quantity_2() == 0);
            CHECK(g_reporter->get_trade_result()[0]->sell_quantity_1() == 0);
            CHECK(g_reporter->get_trade_result()[0]->buy_quantity_1() == 0);
            CHECK(g_reporter->get_trade_result()[0]->buy_quantity_2() == 0);
            CHECK(g_reporter->get_trade_result()[0]->buy_quantity_3() == 0);
        }

        SECTION("Continuous trade after call auction")
        {
            trade::booker::Booker booker({}, reporter());

            booker.add(TickCreator::order_tick(0, LIMIT, "600875.SH", SELL, 2233, 20, 91500000));
            booker.add(TickCreator::order_tick(1, LIMIT, "600875.SH", SELL, 3322, 40, 91500000));
            booker.add(TickCreator::order_tick(2, LIMIT, "600875.SH", SELL, 3333, 80, 91500000));
            booker.add(TickCreator::order_tick(3, LIMIT, "600875.SH", BUY, 2222, 100, 91500000));
            booker.add(TickCreator::order_tick(4, LIMIT, "600875.SH", BUY, 2233, 100, 91500000));
            booker.add(TickCreator::order_tick(5, LIMIT, "600875.SH", BUY, 3322, 100, 91500000));

            booker.trade(TickCreator::trade_tick(1, 5, "600875.SH", 3322, 40, 92500000));
            booker.trade(TickCreator::trade_tick(0, 5, "600875.SH", 3322, 20, 92500000));

            booker.switch_to_continuous_stage();

            booker.add(TickCreator::order_tick(6, MARKET, "600875.SH", BUY, 0, 80, 93000000));
            booker.trade(TickCreator::trade_tick(2, 6, "600875.SH", 3333, 80, 93000000));
            booker.add(TickCreator::order_tick(7, MARKET, "600875.SH", SELL, 0, 200, 93000000));
            booker.trade(TickCreator::trade_tick(7, 5, "600875.SH", 3322, 100, 93000000));
            booker.trade(TickCreator::trade_tick(7, 4, "600875.SH", 2233, 100, 93000000));

            CHECK(g_reporter->get_trade_result().size() == 5);

            CHECK(g_reporter->get_trade_result()[0]->symbol() == "600875.SH");
            CHECK(g_reporter->get_trade_result()[0]->price_1000x() == 3322);
            CHECK(g_reporter->get_trade_result()[0]->quantity() == 40);
            CHECK(g_reporter->get_trade_result()[0]->ask_unique_id() == 1);
            CHECK(g_reporter->get_trade_result()[0]->bid_unique_id() == 5);
            CHECK(g_reporter->get_trade_result()[0]->sell_price_1000x_3() == 0);
            CHECK(g_reporter->get_trade_result()[0]->sell_price_1000x_2() == 0);
            CHECK(g_reporter->get_trade_result()[0]->sell_price_1000x_1() == 0);
            CHECK(g_reporter->get_trade_result()[0]->buy_price_1000x_1() == 0);
            CHECK(g_reporter->get_trade_result()[0]->buy_price_1000x_2() == 0);
            CHECK(g_reporter->get_trade_result()[0]->buy_price_1000x_3() == 0);
            CHECK(g_reporter->get_trade_result()[0]->sell_quantity_3() == 0);
            CHECK(g_reporter->get_trade_result()[0]->sell_quantity_2() == 0);
            CHECK(g_reporter->get_trade_result()[0]->sell_quantity_1() == 0);
            CHECK(g_reporter->get_trade_result()[0]->buy_quantity_1() == 0);
            CHECK(g_reporter->get_trade_result()[0]->buy_quantity_2() == 0);
            CHECK(g_reporter->get_trade_result()[0]->buy_quantity_3() == 0);

            CHECK(g_reporter->get_trade_result()[1]->symbol() == "600875.SH");
            CHECK(g_reporter->get_trade_result()[1]->price_1000x() == 3322);
            CHECK(g_reporter->get_trade_result()[1]->quantity() == 20);
            CHECK(g_reporter->get_trade_result()[1]->ask_unique_id() == 0);
            CHECK(g_reporter->get_trade_result()[1]->bid_unique_id() == 5);
            CHECK(g_reporter->get_trade_result()[1]->sell_price_1000x_3() == 0);
            CHECK(g_reporter->get_trade_result()[1]->sell_price_1000x_2() == 0);
            CHECK(g_reporter->get_trade_result()[1]->sell_price_1000x_1() == 0);
            CHECK(g_reporter->get_trade_result()[1]->buy_price_1000x_1() == 0);
            CHECK(g_reporter->get_trade_result()[1]->buy_price_1000x_2() == 0);
            CHECK(g_reporter->get_trade_result()[1]->buy_price_1000x_3() == 0);
            CHECK(g_reporter->get_trade_result()[1]->sell_quantity_3() == 0);
            CHECK(g_reporter->get_trade_result()[1]->sell_quantity_2() == 0);
            CHECK(g_reporter->get_trade_result()[1]->sell_quantity_1() == 0);
            CHECK(g_reporter->get_trade_result()[1]->buy_quantity_1() == 0);
            CHECK(g_reporter->get_trade_result()[1]->buy_quantity_2() == 0);
            CHECK(g_reporter->get_trade_result()[1]->buy_quantity_3() == 0);

            CHECK(g_reporter->get_trade_result()[2]->symbol() == "600875.SH");
            CHECK(g_reporter->get_trade_result()[2]->price_1000x() == 3333);
            CHECK(g_reporter->get_trade_result()[2]->quantity() == 80);
            CHECK(g_reporter->get_trade_result()[2]->ask_unique_id() == 2);
            CHECK(g_reporter->get_trade_result()[2]->bid_unique_id() == 6);
            CHECK(g_reporter->get_trade_result()[2]->sell_price_1000x_3() == 0);
            CHECK(g_reporter->get_trade_result()[2]->sell_price_1000x_2() == 0);
            CHECK(g_reporter->get_trade_result()[2]->sell_price_1000x_1() == 0);
            CHECK(g_reporter->get_trade_result()[2]->buy_price_1000x_1() == 3322);
            CHECK(g_reporter->get_trade_result()[2]->buy_price_1000x_2() == 0);
            CHECK(g_reporter->get_trade_result()[2]->buy_price_1000x_3() == 0);
            CHECK(g_reporter->get_trade_result()[2]->sell_quantity_3() == 0);
            CHECK(g_reporter->get_trade_result()[2]->sell_quantity_2() == 0);
            CHECK(g_reporter->get_trade_result()[2]->sell_quantity_1() == 0);
            CHECK(g_reporter->get_trade_result()[2]->buy_quantity_1() == 240);
            CHECK(g_reporter->get_trade_result()[2]->buy_quantity_2() == 0);
            CHECK(g_reporter->get_trade_result()[2]->buy_quantity_3() == 0);

            CHECK(g_reporter->get_trade_result()[3]->symbol() == "600875.SH");
            CHECK(g_reporter->get_trade_result()[3]->price_1000x() == 3322);
            CHECK(g_reporter->get_trade_result()[3]->quantity() == 40);
            CHECK(g_reporter->get_trade_result()[3]->ask_unique_id() == 7);
            CHECK(g_reporter->get_trade_result()[3]->bid_unique_id() == 5);
            CHECK(g_reporter->get_trade_result()[3]->sell_price_1000x_3() == 0);
            CHECK(g_reporter->get_trade_result()[3]->sell_price_1000x_2() == 0);
            CHECK(g_reporter->get_trade_result()[3]->sell_price_1000x_1() == 3322);
            CHECK(g_reporter->get_trade_result()[3]->buy_price_1000x_1() == 2233);
            CHECK(g_reporter->get_trade_result()[3]->buy_price_1000x_2() == 0);
            CHECK(g_reporter->get_trade_result()[3]->buy_price_1000x_3() == 0);
            CHECK(g_reporter->get_trade_result()[3]->sell_quantity_3() == 0);
            CHECK(g_reporter->get_trade_result()[3]->sell_quantity_2() == 0);
            CHECK(g_reporter->get_trade_result()[3]->sell_quantity_1() == 60);
            CHECK(g_reporter->get_trade_result()[3]->buy_quantity_1() == 200);
            CHECK(g_reporter->get_trade_result()[3]->buy_quantity_2() == 0);
            CHECK(g_reporter->get_trade_result()[3]->buy_quantity_3() == 0);

            CHECK(g_reporter->get_trade_result()[4]->symbol() == "600875.SH");
            CHECK(g_reporter->get_trade_result()[4]->price_1000x() == 2233);
            CHECK(g_reporter->get_trade_result()[4]->quantity() == 100);
            CHECK(g_reporter->get_trade_result()[4]->ask_unique_id() == 7);
            CHECK(g_reporter->get_trade_result()[4]->bid_unique_id() == 4);
            CHECK(g_reporter->get_trade_result()[4]->sell_price_1000x_3() == 0);
            CHECK(g_reporter->get_trade_result()[4]->sell_price_1000x_2() == 0);
            CHECK(g_reporter->get_trade_result()[4]->sell_price_1000x_1() == 3322);
            CHECK(g_reporter->get_trade_result()[4]->buy_price_1000x_1() == 2222);
            CHECK(g_reporter->get_trade_result()[4]->buy_price_1000x_2() == 0);
            CHECK(g_reporter->get_trade_result()[4]->buy_price_1000x_3() == 0);
            CHECK(g_reporter->get_trade_result()[4]->sell_quantity_3() == 0);
            CHECK(g_reporter->get_trade_result()[4]->sell_quantity_2() == 0);
            CHECK(g_reporter->get_trade_result()[4]->sell_quantity_1() == 60);
            CHECK(g_reporter->get_trade_result()[4]->buy_quantity_1() == 100);
            CHECK(g_reporter->get_trade_result()[4]->buy_quantity_2() == 0);
            CHECK(g_reporter->get_trade_result()[4]->buy_quantity_3() == 0);
        }
    }
}
