#include <catch.hpp>

#include "libbooker/Booker.h"
#include "libreporter/NopReporter.hpp"
#include "utilities/OrderCreator.hpp"

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
    void l2_tick_generated(const std::shared_ptr<trade::types::L2Tick> l2_tick) override
    {
        m_trade_results.push_back(l2_tick);
    }

private:
    std::vector<std::shared_ptr<trade::types::L2Tick>> m_trade_results;
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

            booker.add(OrderCreator::ororder_tick(0, LIMIT, "600875.SH", BUY, 22.33, 100));
            booker.add(OrderCreator::ororder_tick(1, LIMIT, "600875.SH", SELL, 22.33, 100));

            CHECK(g_reporter->get_trade_result().size() == 1);

            CHECK(g_reporter->get_trade_result()[0]->symbol() == "600875.SH");
            CHECK(g_reporter->get_trade_result()[0]->price() == 22.33);
            CHECK(g_reporter->get_trade_result()[0]->quantity() == 100);
            CHECK(g_reporter->get_trade_result()[0]->ask_unique_id() == 1);
            CHECK(g_reporter->get_trade_result()[0]->bid_unique_id() == 0);
            CHECK(g_reporter->get_trade_result()[0]->sell_price_3() == 0);
            CHECK(g_reporter->get_trade_result()[0]->sell_price_2() == 0);
            CHECK(g_reporter->get_trade_result()[0]->sell_price_1() == 0);
            CHECK(g_reporter->get_trade_result()[0]->buy_price_1() == 0);
            CHECK(g_reporter->get_trade_result()[0]->buy_price_2() == 0);
            CHECK(g_reporter->get_trade_result()[0]->buy_price_3() == 0);
        }

        SECTION("Buy after sell with same price")
        {
            trade::booker::Booker booker({}, reporter());

            booker.add(OrderCreator::ororder_tick(0, LIMIT, "600875.SH", SELL, 22.33, 100));
            booker.add(OrderCreator::ororder_tick(1, LIMIT, "600875.SH", BUY, 22.33, 100));

            CHECK(g_reporter->get_trade_result().size() == 1);

            CHECK(g_reporter->get_trade_result()[0]->symbol() == "600875.SH");
            CHECK(g_reporter->get_trade_result()[0]->price() == 22.33);
            CHECK(g_reporter->get_trade_result()[0]->quantity() == 100);
            CHECK(g_reporter->get_trade_result()[0]->ask_unique_id() == 0);
            CHECK(g_reporter->get_trade_result()[0]->bid_unique_id() == 1);
            CHECK(g_reporter->get_trade_result()[0]->sell_price_3() == 0);
            CHECK(g_reporter->get_trade_result()[0]->sell_price_2() == 0);
            CHECK(g_reporter->get_trade_result()[0]->sell_price_1() == 0);
            CHECK(g_reporter->get_trade_result()[0]->buy_price_1() == 0);
            CHECK(g_reporter->get_trade_result()[0]->buy_price_2() == 0);
            CHECK(g_reporter->get_trade_result()[0]->buy_price_3() == 0);
        }

        SECTION("Sell after buy with lower price")
        {
            trade::booker::Booker booker({}, reporter());

            booker.add(OrderCreator::ororder_tick(0, LIMIT, "600875.SH", BUY, 33.22, 100));
            booker.add(OrderCreator::ororder_tick(1, LIMIT, "600875.SH", SELL, 22.33, 100));

            CHECK(g_reporter->get_trade_result().size() == 1);

            CHECK(g_reporter->get_trade_result()[0]->symbol() == "600875.SH");
            CHECK(g_reporter->get_trade_result()[0]->price() == 33.22);
            CHECK(g_reporter->get_trade_result()[0]->quantity() == 100);
            CHECK(g_reporter->get_trade_result()[0]->ask_unique_id() == 1);
            CHECK(g_reporter->get_trade_result()[0]->bid_unique_id() == 0);
            CHECK(g_reporter->get_trade_result()[0]->sell_price_3() == 0);
            CHECK(g_reporter->get_trade_result()[0]->sell_price_2() == 0);
            CHECK(g_reporter->get_trade_result()[0]->sell_price_1() == 0);
            CHECK(g_reporter->get_trade_result()[0]->buy_price_1() == 0);
            CHECK(g_reporter->get_trade_result()[0]->buy_price_2() == 0);
            CHECK(g_reporter->get_trade_result()[0]->buy_price_3() == 0);
        }

        SECTION("Buy after sell with higher price")
        {
            trade::booker::Booker booker({}, reporter());

            booker.add(OrderCreator::ororder_tick(0, LIMIT, "600875.SH", SELL, 22.33, 100));
            booker.add(OrderCreator::ororder_tick(1, LIMIT, "600875.SH", BUY, 33.22, 100));

            CHECK(g_reporter->get_trade_result().size() == 1);

            CHECK(g_reporter->get_trade_result()[0]->symbol() == "600875.SH");
            CHECK(g_reporter->get_trade_result()[0]->price() == 22.33);
            CHECK(g_reporter->get_trade_result()[0]->quantity() == 100);
            CHECK(g_reporter->get_trade_result()[0]->ask_unique_id() == 0);
            CHECK(g_reporter->get_trade_result()[0]->bid_unique_id() == 1);
            CHECK(g_reporter->get_trade_result()[0]->sell_price_3() == 0);
            CHECK(g_reporter->get_trade_result()[0]->sell_price_2() == 0);
            CHECK(g_reporter->get_trade_result()[0]->sell_price_1() == 0);
            CHECK(g_reporter->get_trade_result()[0]->buy_price_1() == 0);
            CHECK(g_reporter->get_trade_result()[0]->buy_price_2() == 0);
            CHECK(g_reporter->get_trade_result()[0]->buy_price_3() == 0);
        }

        SECTION("Sell after buy with higher price")
        {
            trade::booker::Booker booker({}, reporter());

            booker.add(OrderCreator::ororder_tick(0, LIMIT, "600875.SH", BUY, 22.33, 100));
            booker.add(OrderCreator::ororder_tick(1, LIMIT, "600875.SH", SELL, 33.22, 100));

            CHECK(g_reporter->get_trade_result().empty());
        }

        SECTION("Buy after sell with lower price")
        {
            trade::booker::Booker booker({}, reporter());

            booker.add(OrderCreator::ororder_tick(0, LIMIT, "600875.SH", SELL, 33.22, 100));
            booker.add(OrderCreator::ororder_tick(1, LIMIT, "600875.SH", BUY, 22.33, 100));

            CHECK(g_reporter->get_trade_result().empty());
        }
    }

    SECTION("Limit order matching with M:N matching")
    {
        SECTION("Buy after sell with same price")
        {
            trade::booker::Booker booker({}, reporter());

            booker.add(OrderCreator::ororder_tick(0, LIMIT, "600875.SH", SELL, 22.33, 20));
            booker.add(OrderCreator::ororder_tick(1, LIMIT, "600875.SH", SELL, 22.33, 40));
            booker.add(OrderCreator::ororder_tick(2, LIMIT, "600875.SH", SELL, 22.33, 80));
            booker.add(OrderCreator::ororder_tick(3, LIMIT, "600875.SH", BUY, 22.33, 100));
            booker.add(OrderCreator::ororder_tick(4, LIMIT, "600875.SH", BUY, 22.33, 100));

            CHECK(g_reporter->get_trade_result().size() == 4);

            CHECK(g_reporter->get_trade_result()[0]->symbol() == "600875.SH");
            CHECK(g_reporter->get_trade_result()[0]->price() == 22.33);
            CHECK(g_reporter->get_trade_result()[0]->quantity() == 20);
            CHECK(g_reporter->get_trade_result()[0]->ask_unique_id() == 0);
            CHECK(g_reporter->get_trade_result()[0]->bid_unique_id() == 3);
            CHECK(g_reporter->get_trade_result()[0]->sell_price_3() == 0);
            CHECK(g_reporter->get_trade_result()[0]->sell_price_2() == 0);
            CHECK(g_reporter->get_trade_result()[0]->sell_price_1() == 22.33);
            CHECK(g_reporter->get_trade_result()[0]->buy_price_1() == 0);
            CHECK(g_reporter->get_trade_result()[0]->buy_price_2() == 0);
            CHECK(g_reporter->get_trade_result()[0]->buy_price_3() == 0);

            CHECK(g_reporter->get_trade_result()[1]->symbol() == "600875.SH");
            CHECK(g_reporter->get_trade_result()[1]->price() == 22.33);
            CHECK(g_reporter->get_trade_result()[1]->quantity() == 40);
            CHECK(g_reporter->get_trade_result()[1]->ask_unique_id() == 1);
            CHECK(g_reporter->get_trade_result()[1]->bid_unique_id() == 3);
            CHECK(g_reporter->get_trade_result()[1]->sell_price_3() == 0);
            CHECK(g_reporter->get_trade_result()[1]->sell_price_2() == 0);
            CHECK(g_reporter->get_trade_result()[1]->sell_price_1() == 22.33);
            CHECK(g_reporter->get_trade_result()[1]->buy_price_1() == 0);
            CHECK(g_reporter->get_trade_result()[1]->buy_price_2() == 0);
            CHECK(g_reporter->get_trade_result()[1]->buy_price_3() == 0);

            CHECK(g_reporter->get_trade_result()[2]->symbol() == "600875.SH");
            CHECK(g_reporter->get_trade_result()[2]->price() == 22.33);
            CHECK(g_reporter->get_trade_result()[2]->quantity() == 40);
            CHECK(g_reporter->get_trade_result()[2]->ask_unique_id() == 2);
            CHECK(g_reporter->get_trade_result()[2]->bid_unique_id() == 3);
            CHECK(g_reporter->get_trade_result()[2]->sell_price_3() == 0);
            CHECK(g_reporter->get_trade_result()[2]->sell_price_2() == 0);
            CHECK(g_reporter->get_trade_result()[2]->sell_price_1() == 22.33);
            CHECK(g_reporter->get_trade_result()[2]->buy_price_1() == 0);
            CHECK(g_reporter->get_trade_result()[2]->buy_price_2() == 0);
            CHECK(g_reporter->get_trade_result()[2]->buy_price_3() == 0);

            CHECK(g_reporter->get_trade_result()[3]->symbol() == "600875.SH");
            CHECK(g_reporter->get_trade_result()[3]->price() == 22.33);
            CHECK(g_reporter->get_trade_result()[3]->quantity() == 40);
            CHECK(g_reporter->get_trade_result()[3]->ask_unique_id() == 2);
            CHECK(g_reporter->get_trade_result()[3]->bid_unique_id() == 4);
            CHECK(g_reporter->get_trade_result()[3]->sell_price_3() == 0);
            CHECK(g_reporter->get_trade_result()[3]->sell_price_2() == 0);
            CHECK(g_reporter->get_trade_result()[3]->sell_price_1() == 0);
            CHECK(g_reporter->get_trade_result()[3]->buy_price_1() == 22.33);
            CHECK(g_reporter->get_trade_result()[3]->buy_price_2() == 0);
            CHECK(g_reporter->get_trade_result()[3]->buy_price_3() == 0);
        }

        SECTION("Buy after sell with same price")
        {
            trade::booker::Booker booker({}, reporter());

            booker.add(OrderCreator::ororder_tick(0, LIMIT, "600875.SH", SELL, 22.33, 20));
            booker.add(OrderCreator::ororder_tick(1, LIMIT, "600875.SH", SELL, 22.33, 40));
            booker.add(OrderCreator::ororder_tick(2, LIMIT, "600875.SH", SELL, 22.33, 80));
            booker.add(OrderCreator::ororder_tick(3, LIMIT, "600875.SH", BUY, 22.33, 100));
            booker.add(OrderCreator::ororder_tick(4, LIMIT, "600875.SH", BUY, 22.33, 100));

            CHECK(g_reporter->get_trade_result().size() == 4);

            CHECK(g_reporter->get_trade_result()[0]->symbol() == "600875.SH");
            CHECK(g_reporter->get_trade_result()[0]->price() == 22.33);
            CHECK(g_reporter->get_trade_result()[0]->quantity() == 20);
            CHECK(g_reporter->get_trade_result()[0]->ask_unique_id() == 0);
            CHECK(g_reporter->get_trade_result()[0]->bid_unique_id() == 3);
            CHECK(g_reporter->get_trade_result()[0]->sell_price_3() == 0);
            CHECK(g_reporter->get_trade_result()[0]->sell_price_2() == 0);
            CHECK(g_reporter->get_trade_result()[0]->sell_price_1() == 22.33);
            CHECK(g_reporter->get_trade_result()[0]->buy_price_1() == 0);
            CHECK(g_reporter->get_trade_result()[0]->buy_price_2() == 0);
            CHECK(g_reporter->get_trade_result()[0]->buy_price_3() == 0);

            CHECK(g_reporter->get_trade_result()[1]->symbol() == "600875.SH");
            CHECK(g_reporter->get_trade_result()[1]->price() == 22.33);
            CHECK(g_reporter->get_trade_result()[1]->quantity() == 40);
            CHECK(g_reporter->get_trade_result()[1]->ask_unique_id() == 1);
            CHECK(g_reporter->get_trade_result()[1]->bid_unique_id() == 3);
            CHECK(g_reporter->get_trade_result()[1]->sell_price_3() == 0);
            CHECK(g_reporter->get_trade_result()[1]->sell_price_2() == 0);
            CHECK(g_reporter->get_trade_result()[1]->sell_price_1() == 22.33);
            CHECK(g_reporter->get_trade_result()[1]->buy_price_1() == 0);
            CHECK(g_reporter->get_trade_result()[1]->buy_price_2() == 0);
            CHECK(g_reporter->get_trade_result()[1]->buy_price_3() == 0);

            CHECK(g_reporter->get_trade_result()[2]->symbol() == "600875.SH");
            CHECK(g_reporter->get_trade_result()[2]->price() == 22.33);
            CHECK(g_reporter->get_trade_result()[2]->quantity() == 40);
            CHECK(g_reporter->get_trade_result()[2]->ask_unique_id() == 2);
            CHECK(g_reporter->get_trade_result()[2]->bid_unique_id() == 3);
            CHECK(g_reporter->get_trade_result()[2]->sell_price_3() == 0);
            CHECK(g_reporter->get_trade_result()[2]->sell_price_2() == 0);
            CHECK(g_reporter->get_trade_result()[2]->sell_price_1() == 22.33);
            CHECK(g_reporter->get_trade_result()[2]->buy_price_1() == 0);
            CHECK(g_reporter->get_trade_result()[2]->buy_price_2() == 0);
            CHECK(g_reporter->get_trade_result()[2]->buy_price_3() == 0);

            CHECK(g_reporter->get_trade_result()[3]->symbol() == "600875.SH");
            CHECK(g_reporter->get_trade_result()[3]->price() == 22.33);
            CHECK(g_reporter->get_trade_result()[3]->quantity() == 40);
            CHECK(g_reporter->get_trade_result()[3]->ask_unique_id() == 2);
            CHECK(g_reporter->get_trade_result()[3]->bid_unique_id() == 4);
            CHECK(g_reporter->get_trade_result()[3]->sell_price_3() == 0);
            CHECK(g_reporter->get_trade_result()[3]->sell_price_2() == 0);
            CHECK(g_reporter->get_trade_result()[3]->sell_price_1() == 0);
            CHECK(g_reporter->get_trade_result()[3]->buy_price_1() == 22.33);
            CHECK(g_reporter->get_trade_result()[3]->buy_price_2() == 0);
            CHECK(g_reporter->get_trade_result()[3]->buy_price_3() == 0);
        }

        SECTION("Sell after buy with lower price")
        {
            trade::booker::Booker booker({}, reporter());

            booker.add(OrderCreator::ororder_tick(0, LIMIT, "600875.SH", BUY, 33.22, 20));
            booker.add(OrderCreator::ororder_tick(1, LIMIT, "600875.SH", BUY, 33.22, 40));
            booker.add(OrderCreator::ororder_tick(2, LIMIT, "600875.SH", BUY, 33.22, 80));
            booker.add(OrderCreator::ororder_tick(3, LIMIT, "600875.SH", SELL, 22.33, 100));
            booker.add(OrderCreator::ororder_tick(4, LIMIT, "600875.SH", SELL, 22.33, 100));

            CHECK(g_reporter->get_trade_result().size() == 4);

            CHECK(g_reporter->get_trade_result()[0]->symbol() == "600875.SH");
            CHECK(g_reporter->get_trade_result()[0]->price() == 33.22);
            CHECK(g_reporter->get_trade_result()[0]->quantity() == 20);
            CHECK(g_reporter->get_trade_result()[0]->ask_unique_id() == 3);
            CHECK(g_reporter->get_trade_result()[0]->bid_unique_id() == 0);
            CHECK(g_reporter->get_trade_result()[0]->sell_price_3() == 0);
            CHECK(g_reporter->get_trade_result()[0]->sell_price_2() == 0);
            CHECK(g_reporter->get_trade_result()[0]->sell_price_1() == 0);
            CHECK(g_reporter->get_trade_result()[0]->buy_price_1() == 33.22);
            CHECK(g_reporter->get_trade_result()[0]->buy_price_2() == 0);
            CHECK(g_reporter->get_trade_result()[0]->buy_price_3() == 0);

            CHECK(g_reporter->get_trade_result()[1]->symbol() == "600875.SH");
            CHECK(g_reporter->get_trade_result()[1]->price() == 33.22);
            CHECK(g_reporter->get_trade_result()[1]->quantity() == 40);
            CHECK(g_reporter->get_trade_result()[1]->ask_unique_id() == 3);
            CHECK(g_reporter->get_trade_result()[1]->bid_unique_id() == 1);
            CHECK(g_reporter->get_trade_result()[1]->sell_price_3() == 0);
            CHECK(g_reporter->get_trade_result()[1]->sell_price_2() == 0);
            CHECK(g_reporter->get_trade_result()[1]->sell_price_1() == 0);
            CHECK(g_reporter->get_trade_result()[1]->buy_price_1() == 33.22);
            CHECK(g_reporter->get_trade_result()[1]->buy_price_2() == 0);
            CHECK(g_reporter->get_trade_result()[1]->buy_price_3() == 0);

            CHECK(g_reporter->get_trade_result()[2]->symbol() == "600875.SH");
            CHECK(g_reporter->get_trade_result()[2]->price() == 33.22);
            CHECK(g_reporter->get_trade_result()[2]->quantity() == 40);
            CHECK(g_reporter->get_trade_result()[2]->ask_unique_id() == 3);
            CHECK(g_reporter->get_trade_result()[2]->bid_unique_id() == 2);
            CHECK(g_reporter->get_trade_result()[2]->sell_price_3() == 0);
            CHECK(g_reporter->get_trade_result()[2]->sell_price_2() == 0);
            CHECK(g_reporter->get_trade_result()[2]->sell_price_1() == 0);
            CHECK(g_reporter->get_trade_result()[2]->buy_price_1() == 33.22);
            CHECK(g_reporter->get_trade_result()[2]->buy_price_2() == 0);
            CHECK(g_reporter->get_trade_result()[2]->buy_price_3() == 0);

            CHECK(g_reporter->get_trade_result()[3]->symbol() == "600875.SH");
            CHECK(g_reporter->get_trade_result()[3]->price() == 33.22);
            CHECK(g_reporter->get_trade_result()[3]->quantity() == 40);
            CHECK(g_reporter->get_trade_result()[3]->ask_unique_id() == 4);
            CHECK(g_reporter->get_trade_result()[3]->bid_unique_id() == 2);
            CHECK(g_reporter->get_trade_result()[3]->sell_price_3() == 0);
            CHECK(g_reporter->get_trade_result()[3]->sell_price_2() == 0);
            CHECK(g_reporter->get_trade_result()[3]->sell_price_1() == 22.33);
            CHECK(g_reporter->get_trade_result()[3]->buy_price_1() == 0);
            CHECK(g_reporter->get_trade_result()[3]->buy_price_2() == 0);
            CHECK(g_reporter->get_trade_result()[3]->buy_price_3() == 0);
        }

        SECTION("Buy after sell with higher price")
        {
            trade::booker::Booker booker({}, reporter());

            booker.add(OrderCreator::ororder_tick(0, LIMIT, "600875.SH", SELL, 22.33, 20));
            booker.add(OrderCreator::ororder_tick(1, LIMIT, "600875.SH", SELL, 22.33, 40));
            booker.add(OrderCreator::ororder_tick(2, LIMIT, "600875.SH", SELL, 22.33, 80));
            booker.add(OrderCreator::ororder_tick(3, LIMIT, "600875.SH", BUY, 33.22, 100));
            booker.add(OrderCreator::ororder_tick(4, LIMIT, "600875.SH", BUY, 33.22, 100));

            CHECK(g_reporter->get_trade_result().size() == 4);

            CHECK(g_reporter->get_trade_result()[0]->symbol() == "600875.SH");
            CHECK(g_reporter->get_trade_result()[0]->price() == 22.33);
            CHECK(g_reporter->get_trade_result()[0]->quantity() == 20);
            CHECK(g_reporter->get_trade_result()[0]->ask_unique_id() == 0);
            CHECK(g_reporter->get_trade_result()[0]->bid_unique_id() == 3);
            CHECK(g_reporter->get_trade_result()[0]->sell_price_3() == 0);
            CHECK(g_reporter->get_trade_result()[0]->sell_price_2() == 0);
            CHECK(g_reporter->get_trade_result()[0]->sell_price_1() == 22.33);
            CHECK(g_reporter->get_trade_result()[0]->buy_price_1() == 0);
            CHECK(g_reporter->get_trade_result()[0]->buy_price_2() == 0);
            CHECK(g_reporter->get_trade_result()[0]->buy_price_3() == 0);

            CHECK(g_reporter->get_trade_result()[1]->symbol() == "600875.SH");
            CHECK(g_reporter->get_trade_result()[1]->price() == 22.33);
            CHECK(g_reporter->get_trade_result()[1]->quantity() == 40);
            CHECK(g_reporter->get_trade_result()[1]->ask_unique_id() == 1);
            CHECK(g_reporter->get_trade_result()[1]->bid_unique_id() == 3);
            CHECK(g_reporter->get_trade_result()[1]->sell_price_3() == 0);
            CHECK(g_reporter->get_trade_result()[1]->sell_price_2() == 0);
            CHECK(g_reporter->get_trade_result()[1]->sell_price_1() == 22.33);
            CHECK(g_reporter->get_trade_result()[1]->buy_price_1() == 0);
            CHECK(g_reporter->get_trade_result()[1]->buy_price_2() == 0);
            CHECK(g_reporter->get_trade_result()[1]->buy_price_3() == 0);

            CHECK(g_reporter->get_trade_result()[2]->symbol() == "600875.SH");
            CHECK(g_reporter->get_trade_result()[2]->price() == 22.33);
            CHECK(g_reporter->get_trade_result()[2]->quantity() == 40);
            CHECK(g_reporter->get_trade_result()[2]->ask_unique_id() == 2);
            CHECK(g_reporter->get_trade_result()[2]->bid_unique_id() == 3);
            CHECK(g_reporter->get_trade_result()[2]->sell_price_3() == 0);
            CHECK(g_reporter->get_trade_result()[2]->sell_price_2() == 0);
            CHECK(g_reporter->get_trade_result()[2]->sell_price_1() == 22.33);
            CHECK(g_reporter->get_trade_result()[2]->buy_price_1() == 0);
            CHECK(g_reporter->get_trade_result()[2]->buy_price_2() == 0);
            CHECK(g_reporter->get_trade_result()[2]->buy_price_3() == 0);

            CHECK(g_reporter->get_trade_result()[3]->symbol() == "600875.SH");
            CHECK(g_reporter->get_trade_result()[3]->price() == 22.33);
            CHECK(g_reporter->get_trade_result()[3]->quantity() == 40);
            CHECK(g_reporter->get_trade_result()[3]->ask_unique_id() == 2);
            CHECK(g_reporter->get_trade_result()[3]->bid_unique_id() == 4);
            CHECK(g_reporter->get_trade_result()[3]->sell_price_3() == 0);
            CHECK(g_reporter->get_trade_result()[3]->sell_price_2() == 0);
            CHECK(g_reporter->get_trade_result()[3]->sell_price_1() == 0);
            CHECK(g_reporter->get_trade_result()[3]->buy_price_1() == 33.22);
            CHECK(g_reporter->get_trade_result()[3]->buy_price_2() == 0);
            CHECK(g_reporter->get_trade_result()[3]->buy_price_3() == 0);
        }

        SECTION("Sell after buy with step price")
        {
            trade::booker::Booker booker({}, reporter());

            booker.add(OrderCreator::ororder_tick(0, LIMIT, "600875.SH", BUY, 22.33, 20));
            booker.add(OrderCreator::ororder_tick(1, LIMIT, "600875.SH", BUY, 33.22, 40));
            booker.add(OrderCreator::ororder_tick(2, LIMIT, "600875.SH", BUY, 33.33, 80));
            booker.add(OrderCreator::ororder_tick(3, LIMIT, "600875.SH", SELL, 33.22, 100));
            booker.add(OrderCreator::ororder_tick(4, LIMIT, "600875.SH", SELL, 33.22, 100));

            CHECK(g_reporter->get_trade_result().size() == 3);

            CHECK(g_reporter->get_trade_result()[0]->symbol() == "600875.SH");
            CHECK(g_reporter->get_trade_result()[0]->price() == 33.33);
            CHECK(g_reporter->get_trade_result()[0]->quantity() == 80);
            CHECK(g_reporter->get_trade_result()[0]->ask_unique_id() == 3);
            CHECK(g_reporter->get_trade_result()[0]->bid_unique_id() == 2);
            CHECK(g_reporter->get_trade_result()[0]->sell_price_3() == 0);
            CHECK(g_reporter->get_trade_result()[0]->sell_price_2() == 0);
            CHECK(g_reporter->get_trade_result()[0]->sell_price_1() == 0);
            CHECK(g_reporter->get_trade_result()[0]->buy_price_1() == 33.22);
            CHECK(g_reporter->get_trade_result()[0]->buy_price_2() == 22.33);
            CHECK(g_reporter->get_trade_result()[0]->buy_price_3() == 0);

            CHECK(g_reporter->get_trade_result()[1]->symbol() == "600875.SH");
            CHECK(g_reporter->get_trade_result()[1]->price() == 33.22);
            CHECK(g_reporter->get_trade_result()[1]->quantity() == 20);
            CHECK(g_reporter->get_trade_result()[1]->ask_unique_id() == 3);
            CHECK(g_reporter->get_trade_result()[1]->bid_unique_id() == 1);
            CHECK(g_reporter->get_trade_result()[1]->sell_price_3() == 0);
            CHECK(g_reporter->get_trade_result()[1]->sell_price_2() == 0);
            CHECK(g_reporter->get_trade_result()[1]->sell_price_1() == 0);
            CHECK(g_reporter->get_trade_result()[1]->buy_price_1() == 33.22);
            CHECK(g_reporter->get_trade_result()[1]->buy_price_2() == 22.33);
            CHECK(g_reporter->get_trade_result()[1]->buy_price_3() == 0);

            CHECK(g_reporter->get_trade_result()[2]->symbol() == "600875.SH");
            CHECK(g_reporter->get_trade_result()[2]->price() == 33.22);
            CHECK(g_reporter->get_trade_result()[2]->quantity() == 20);
            CHECK(g_reporter->get_trade_result()[2]->ask_unique_id() == 4);
            CHECK(g_reporter->get_trade_result()[2]->bid_unique_id() == 1);
            CHECK(g_reporter->get_trade_result()[2]->sell_price_3() == 0);
            CHECK(g_reporter->get_trade_result()[2]->sell_price_2() == 0);
            CHECK(g_reporter->get_trade_result()[2]->sell_price_1() == 33.22);
            CHECK(g_reporter->get_trade_result()[2]->buy_price_1() == 22.33);
            CHECK(g_reporter->get_trade_result()[2]->buy_price_2() == 0);
            CHECK(g_reporter->get_trade_result()[2]->buy_price_3() == 0);
        }

        SECTION("Buy after sell with step price")
        {
            trade::booker::Booker booker({}, reporter());

            booker.add(OrderCreator::ororder_tick(0, LIMIT, "600875.SH", SELL, 22.22, 20));
            booker.add(OrderCreator::ororder_tick(1, LIMIT, "600875.SH", SELL, 22.33, 40));
            booker.add(OrderCreator::ororder_tick(2, LIMIT, "600875.SH", SELL, 33.22, 80));
            booker.add(OrderCreator::ororder_tick(3, LIMIT, "600875.SH", BUY, 22.33, 100));
            booker.add(OrderCreator::ororder_tick(4, LIMIT, "600875.SH", BUY, 22.33, 100));

            CHECK(g_reporter->get_trade_result().size() == 2);

            CHECK(g_reporter->get_trade_result()[0]->symbol() == "600875.SH");
            CHECK(g_reporter->get_trade_result()[0]->price() == 22.22);
            CHECK(g_reporter->get_trade_result()[0]->quantity() == 20);
            CHECK(g_reporter->get_trade_result()[0]->ask_unique_id() == 0);
            CHECK(g_reporter->get_trade_result()[0]->bid_unique_id() == 3);
            CHECK(g_reporter->get_trade_result()[0]->sell_price_3() == 0);
            CHECK(g_reporter->get_trade_result()[0]->sell_price_2() == 0);
            CHECK(g_reporter->get_trade_result()[0]->sell_price_1() == 33.22);
            CHECK(g_reporter->get_trade_result()[0]->buy_price_1() == 22.33);
            CHECK(g_reporter->get_trade_result()[0]->buy_price_2() == 0);
            CHECK(g_reporter->get_trade_result()[0]->buy_price_3() == 0);

            CHECK(g_reporter->get_trade_result()[1]->symbol() == "600875.SH");
            CHECK(g_reporter->get_trade_result()[1]->price() == 22.33);
            CHECK(g_reporter->get_trade_result()[1]->quantity() == 40);
            CHECK(g_reporter->get_trade_result()[1]->ask_unique_id() == 1);
            CHECK(g_reporter->get_trade_result()[1]->bid_unique_id() == 3);
            CHECK(g_reporter->get_trade_result()[1]->sell_price_3() == 0);
            CHECK(g_reporter->get_trade_result()[1]->sell_price_2() == 0);
            CHECK(g_reporter->get_trade_result()[1]->sell_price_1() == 33.22);
            CHECK(g_reporter->get_trade_result()[1]->buy_price_1() == 22.33);
            CHECK(g_reporter->get_trade_result()[1]->buy_price_2() == 0);
            CHECK(g_reporter->get_trade_result()[1]->buy_price_3() == 0);
        }
    }

    SECTION("Limit order with cancel")
    {
        /// Canceling only requires unique_id.

        SECTION("Limit order with cancel before fill")
        {
            trade::booker::Booker booker({}, reporter());

            booker.add(OrderCreator::ororder_tick(0, LIMIT, "600875.SH", BUY, 22.33, 50));
            booker.add(OrderCreator::ororder_tick(0, CANCEL, "600875.SH", INV_SIDE, 0, 0));
            booker.add(OrderCreator::ororder_tick(1, LIMIT, "600875.SH", SELL, 22.33, 100));

            CHECK(g_reporter->get_trade_result().empty());
        }

        SECTION("Limit order with cancel after partial fill")
        {
            trade::booker::Booker booker({}, reporter());

            booker.add(OrderCreator::ororder_tick(0, LIMIT, "600875.SH", BUY, 22.33, 100));
            booker.add(OrderCreator::ororder_tick(1, LIMIT, "600875.SH", SELL, 22.33, 50));
            booker.add(OrderCreator::ororder_tick(0, CANCEL, "600875.SH", INV_SIDE, 0, 0));
            booker.add(OrderCreator::ororder_tick(2, LIMIT, "600875.SH", SELL, 22.33, 50));

            CHECK(g_reporter->get_trade_result().size() == 1);

            CHECK(g_reporter->get_trade_result()[0]->symbol() == "600875.SH");
            CHECK(g_reporter->get_trade_result()[0]->price() == 22.33);
            CHECK(g_reporter->get_trade_result()[0]->quantity() == 50);
            CHECK(g_reporter->get_trade_result()[0]->ask_unique_id() == 1);
            CHECK(g_reporter->get_trade_result()[0]->bid_unique_id() == 0);
            CHECK(g_reporter->get_trade_result()[0]->sell_price_3() == 0);
            CHECK(g_reporter->get_trade_result()[0]->sell_price_2() == 0);
            CHECK(g_reporter->get_trade_result()[0]->sell_price_1() == 0);
            CHECK(g_reporter->get_trade_result()[0]->buy_price_1() == 22.33);
            CHECK(g_reporter->get_trade_result()[0]->buy_price_2() == 0);
            CHECK(g_reporter->get_trade_result()[0]->buy_price_3() == 0);
        }

        SECTION("Limit order with cancel after full fill")
        {
            trade::booker::Booker booker({}, reporter());

            booker.add(OrderCreator::ororder_tick(0, LIMIT, "600875.SH", BUY, 22.33, 100));
            booker.add(OrderCreator::ororder_tick(1, LIMIT, "600875.SH", SELL, 22.33, 100));
            booker.add(OrderCreator::ororder_tick(0, CANCEL, "600875.SH", INV_SIDE, 0, 0));

            CHECK(g_reporter->get_trade_result().size() == 1);

            CHECK(g_reporter->get_trade_result()[0]->symbol() == "600875.SH");
            CHECK(g_reporter->get_trade_result()[0]->price() == 22.33);
            CHECK(g_reporter->get_trade_result()[0]->quantity() == 100);
            CHECK(g_reporter->get_trade_result()[0]->ask_unique_id() == 1);
            CHECK(g_reporter->get_trade_result()[0]->bid_unique_id() == 0);
            CHECK(g_reporter->get_trade_result()[0]->sell_price_3() == 0);
            CHECK(g_reporter->get_trade_result()[0]->sell_price_2() == 0);
            CHECK(g_reporter->get_trade_result()[0]->sell_price_1() == 0);
            CHECK(g_reporter->get_trade_result()[0]->buy_price_1() == 0);
            CHECK(g_reporter->get_trade_result()[0]->buy_price_2() == 0);
            CHECK(g_reporter->get_trade_result()[0]->buy_price_3() == 0);
        }
    }

    SECTION("Market order matching with 1:1 matching")
    {
        SECTION("Sell after buy with market price")
        {
            trade::booker::Booker booker({}, reporter());

            booker.add(OrderCreator::ororder_tick(0, LIMIT, "600875.SH", BUY, 22.33, 100));
            booker.add(OrderCreator::ororder_tick(1, LIMIT, "600875.SH", BUY, 33.22, 100));
            booker.add(OrderCreator::ororder_tick(2, MARKET, "600875.SH", SELL, 0, 100));

            CHECK(g_reporter->get_trade_result().size() == 1);

            CHECK(g_reporter->get_trade_result()[0]->symbol() == "600875.SH");
            CHECK(g_reporter->get_trade_result()[0]->price() == 33.22);
            CHECK(g_reporter->get_trade_result()[0]->quantity() == 100);
            CHECK(g_reporter->get_trade_result()[0]->ask_unique_id() == 2);
            CHECK(g_reporter->get_trade_result()[0]->bid_unique_id() == 1);
            CHECK(g_reporter->get_trade_result()[0]->sell_price_3() == 0);
            CHECK(g_reporter->get_trade_result()[0]->sell_price_2() == 0);
            CHECK(g_reporter->get_trade_result()[0]->sell_price_1() == 0);
            CHECK(g_reporter->get_trade_result()[0]->buy_price_1() == 22.33);
            CHECK(g_reporter->get_trade_result()[0]->buy_price_2() == 0);
            CHECK(g_reporter->get_trade_result()[0]->buy_price_3() == 0);
        }

        SECTION("Buy after sell with market price")
        {
            trade::booker::Booker booker({}, reporter());

            booker.add(OrderCreator::ororder_tick(0, LIMIT, "600875.SH", SELL, 22.33, 100));
            booker.add(OrderCreator::ororder_tick(1, LIMIT, "600875.SH", SELL, 33.22, 100));
            booker.add(OrderCreator::ororder_tick(2, MARKET, "600875.SH", BUY, 0, 100));

            CHECK(g_reporter->get_trade_result().size() == 1);

            CHECK(g_reporter->get_trade_result()[0]->symbol() == "600875.SH");
            CHECK(g_reporter->get_trade_result()[0]->price() == 22.33);
            CHECK(g_reporter->get_trade_result()[0]->quantity() == 100);
            CHECK(g_reporter->get_trade_result()[0]->ask_unique_id() == 0);
            CHECK(g_reporter->get_trade_result()[0]->bid_unique_id() == 2);
            CHECK(g_reporter->get_trade_result()[0]->sell_price_3() == 0);
            CHECK(g_reporter->get_trade_result()[0]->sell_price_2() == 0);
            CHECK(g_reporter->get_trade_result()[0]->sell_price_1() == 33.22);
            CHECK(g_reporter->get_trade_result()[0]->buy_price_1() == 0);
            CHECK(g_reporter->get_trade_result()[0]->buy_price_2() == 0);
            CHECK(g_reporter->get_trade_result()[0]->buy_price_3() == 0);
        }
    }

    SECTION("Market order matching with M:N matching")
    {
        SECTION("Buy after sell with market price")
        {
            trade::booker::Booker booker({}, reporter());

            booker.add(OrderCreator::ororder_tick(0, LIMIT, "600875.SH", SELL, 22.33, 20));
            booker.add(OrderCreator::ororder_tick(1, LIMIT, "600875.SH", SELL, 22.33, 40));
            booker.add(OrderCreator::ororder_tick(2, LIMIT, "600875.SH", SELL, 22.33, 80));
            booker.add(OrderCreator::ororder_tick(3, MARKET, "600875.SH", BUY, 0, 100));
            booker.add(OrderCreator::ororder_tick(4, MARKET, "600875.SH", BUY, 0, 100));

            CHECK(g_reporter->get_trade_result().size() == 4);

            CHECK(g_reporter->get_trade_result()[0]->symbol() == "600875.SH");
            CHECK(g_reporter->get_trade_result()[0]->price() == 22.33);
            CHECK(g_reporter->get_trade_result()[0]->quantity() == 20);
            CHECK(g_reporter->get_trade_result()[0]->ask_unique_id() == 0);
            CHECK(g_reporter->get_trade_result()[0]->bid_unique_id() == 3);
            CHECK(g_reporter->get_trade_result()[0]->sell_price_3() == 0);
            CHECK(g_reporter->get_trade_result()[0]->sell_price_2() == 0);
            CHECK(g_reporter->get_trade_result()[0]->sell_price_1() == 22.33);
            CHECK(g_reporter->get_trade_result()[0]->buy_price_1() == 0);
            CHECK(g_reporter->get_trade_result()[0]->buy_price_2() == 0);
            CHECK(g_reporter->get_trade_result()[0]->buy_price_3() == 0);

            CHECK(g_reporter->get_trade_result()[1]->symbol() == "600875.SH");
            CHECK(g_reporter->get_trade_result()[1]->price() == 22.33);
            CHECK(g_reporter->get_trade_result()[1]->quantity() == 40);
            CHECK(g_reporter->get_trade_result()[1]->ask_unique_id() == 1);
            CHECK(g_reporter->get_trade_result()[1]->bid_unique_id() == 3);
            CHECK(g_reporter->get_trade_result()[1]->sell_price_3() == 0);
            CHECK(g_reporter->get_trade_result()[1]->sell_price_2() == 0);
            CHECK(g_reporter->get_trade_result()[1]->sell_price_1() == 22.33);
            CHECK(g_reporter->get_trade_result()[1]->buy_price_1() == 0);
            CHECK(g_reporter->get_trade_result()[1]->buy_price_2() == 0);
            CHECK(g_reporter->get_trade_result()[1]->buy_price_3() == 0);

            CHECK(g_reporter->get_trade_result()[2]->symbol() == "600875.SH");
            CHECK(g_reporter->get_trade_result()[2]->price() == 22.33);
            CHECK(g_reporter->get_trade_result()[2]->quantity() == 40);
            CHECK(g_reporter->get_trade_result()[2]->ask_unique_id() == 2);
            CHECK(g_reporter->get_trade_result()[2]->bid_unique_id() == 3);
            CHECK(g_reporter->get_trade_result()[2]->sell_price_3() == 0);
            CHECK(g_reporter->get_trade_result()[2]->sell_price_2() == 0);
            CHECK(g_reporter->get_trade_result()[2]->sell_price_1() == 22.33);
            CHECK(g_reporter->get_trade_result()[2]->buy_price_1() == 0);
            CHECK(g_reporter->get_trade_result()[2]->buy_price_2() == 0);
            CHECK(g_reporter->get_trade_result()[2]->buy_price_3() == 0);

            CHECK(g_reporter->get_trade_result()[3]->symbol() == "600875.SH");
            CHECK(g_reporter->get_trade_result()[3]->price() == 22.33);
            CHECK(g_reporter->get_trade_result()[3]->quantity() == 40);
            CHECK(g_reporter->get_trade_result()[3]->ask_unique_id() == 2);
            CHECK(g_reporter->get_trade_result()[3]->bid_unique_id() == 4);
            CHECK(g_reporter->get_trade_result()[3]->sell_price_3() == 0);
            CHECK(g_reporter->get_trade_result()[3]->sell_price_2() == 0);
            CHECK(g_reporter->get_trade_result()[3]->sell_price_1() == 0);
            CHECK(g_reporter->get_trade_result()[3]->buy_price_1() == 0);
            CHECK(g_reporter->get_trade_result()[3]->buy_price_2() == 0);
            CHECK(g_reporter->get_trade_result()[3]->buy_price_3() == 0);
        }

        SECTION("Buy after sell with market price")
        {
            trade::booker::Booker booker({}, reporter());

            booker.add(OrderCreator::ororder_tick(0, LIMIT, "600875.SH", SELL, 22.33, 20));
            booker.add(OrderCreator::ororder_tick(1, LIMIT, "600875.SH", SELL, 22.33, 40));
            booker.add(OrderCreator::ororder_tick(2, LIMIT, "600875.SH", SELL, 22.33, 80));
            booker.add(OrderCreator::ororder_tick(3, MARKET, "600875.SH", BUY, 0, 100));
            booker.add(OrderCreator::ororder_tick(4, MARKET, "600875.SH", BUY, 0, 100));

            CHECK(g_reporter->get_trade_result().size() == 4);

            CHECK(g_reporter->get_trade_result()[0]->symbol() == "600875.SH");
            CHECK(g_reporter->get_trade_result()[0]->price() == 22.33);
            CHECK(g_reporter->get_trade_result()[0]->quantity() == 20);
            CHECK(g_reporter->get_trade_result()[0]->ask_unique_id() == 0);
            CHECK(g_reporter->get_trade_result()[0]->bid_unique_id() == 3);
            CHECK(g_reporter->get_trade_result()[0]->sell_price_3() == 0);
            CHECK(g_reporter->get_trade_result()[0]->sell_price_2() == 0);
            CHECK(g_reporter->get_trade_result()[0]->sell_price_1() == 22.33);
            CHECK(g_reporter->get_trade_result()[0]->buy_price_1() == 0);
            CHECK(g_reporter->get_trade_result()[0]->buy_price_2() == 0);
            CHECK(g_reporter->get_trade_result()[0]->buy_price_3() == 0);

            CHECK(g_reporter->get_trade_result()[1]->symbol() == "600875.SH");
            CHECK(g_reporter->get_trade_result()[1]->price() == 22.33);
            CHECK(g_reporter->get_trade_result()[1]->quantity() == 40);
            CHECK(g_reporter->get_trade_result()[1]->ask_unique_id() == 1);
            CHECK(g_reporter->get_trade_result()[1]->bid_unique_id() == 3);
            CHECK(g_reporter->get_trade_result()[1]->sell_price_3() == 0);
            CHECK(g_reporter->get_trade_result()[1]->sell_price_2() == 0);
            CHECK(g_reporter->get_trade_result()[1]->sell_price_1() == 22.33);
            CHECK(g_reporter->get_trade_result()[1]->buy_price_1() == 0);
            CHECK(g_reporter->get_trade_result()[1]->buy_price_2() == 0);
            CHECK(g_reporter->get_trade_result()[1]->buy_price_3() == 0);

            CHECK(g_reporter->get_trade_result()[2]->symbol() == "600875.SH");
            CHECK(g_reporter->get_trade_result()[2]->price() == 22.33);
            CHECK(g_reporter->get_trade_result()[2]->quantity() == 40);
            CHECK(g_reporter->get_trade_result()[2]->ask_unique_id() == 2);
            CHECK(g_reporter->get_trade_result()[2]->bid_unique_id() == 3);
            CHECK(g_reporter->get_trade_result()[2]->sell_price_3() == 0);
            CHECK(g_reporter->get_trade_result()[2]->sell_price_2() == 0);
            CHECK(g_reporter->get_trade_result()[2]->sell_price_1() == 22.33);
            CHECK(g_reporter->get_trade_result()[2]->buy_price_1() == 0);
            CHECK(g_reporter->get_trade_result()[2]->buy_price_2() == 0);
            CHECK(g_reporter->get_trade_result()[2]->buy_price_3() == 0);

            CHECK(g_reporter->get_trade_result()[3]->symbol() == "600875.SH");
            CHECK(g_reporter->get_trade_result()[3]->price() == 22.33);
            CHECK(g_reporter->get_trade_result()[3]->quantity() == 40);
            CHECK(g_reporter->get_trade_result()[3]->ask_unique_id() == 2);
            CHECK(g_reporter->get_trade_result()[3]->bid_unique_id() == 4);
            CHECK(g_reporter->get_trade_result()[3]->sell_price_3() == 0);
            CHECK(g_reporter->get_trade_result()[3]->sell_price_2() == 0);
            CHECK(g_reporter->get_trade_result()[3]->sell_price_1() == 0);
            CHECK(g_reporter->get_trade_result()[3]->buy_price_1() == 0);
            CHECK(g_reporter->get_trade_result()[3]->buy_price_2() == 0);
            CHECK(g_reporter->get_trade_result()[3]->buy_price_3() == 0);
        }

        SECTION("Sell after buy with step market price")
        {
            trade::booker::Booker booker({}, reporter());

            booker.add(OrderCreator::ororder_tick(0, LIMIT, "600875.SH", BUY, 22.33, 20));
            booker.add(OrderCreator::ororder_tick(1, LIMIT, "600875.SH", BUY, 33.22, 40));
            booker.add(OrderCreator::ororder_tick(2, LIMIT, "600875.SH", BUY, 33.33, 80));
            booker.add(OrderCreator::ororder_tick(3, MARKET, "600875.SH", SELL, 0, 100));
            booker.add(OrderCreator::ororder_tick(4, MARKET, "600875.SH", SELL, 0, 100));

            CHECK(g_reporter->get_trade_result().size() == 4);

            CHECK(g_reporter->get_trade_result()[0]->symbol() == "600875.SH");
            CHECK(g_reporter->get_trade_result()[0]->price() == 33.33);
            CHECK(g_reporter->get_trade_result()[0]->quantity() == 80);
            CHECK(g_reporter->get_trade_result()[0]->ask_unique_id() == 3);
            CHECK(g_reporter->get_trade_result()[0]->bid_unique_id() == 2);
            CHECK(g_reporter->get_trade_result()[0]->sell_price_3() == 0);
            CHECK(g_reporter->get_trade_result()[0]->sell_price_2() == 0);
            CHECK(g_reporter->get_trade_result()[0]->sell_price_1() == 0);
            CHECK(g_reporter->get_trade_result()[0]->buy_price_1() == 33.22);
            CHECK(g_reporter->get_trade_result()[0]->buy_price_2() == 22.33);
            CHECK(g_reporter->get_trade_result()[0]->buy_price_3() == 0);

            CHECK(g_reporter->get_trade_result()[1]->symbol() == "600875.SH");
            CHECK(g_reporter->get_trade_result()[1]->price() == 33.22);
            CHECK(g_reporter->get_trade_result()[1]->quantity() == 20);
            CHECK(g_reporter->get_trade_result()[1]->ask_unique_id() == 3);
            CHECK(g_reporter->get_trade_result()[1]->bid_unique_id() == 1);
            CHECK(g_reporter->get_trade_result()[1]->sell_price_3() == 0);
            CHECK(g_reporter->get_trade_result()[1]->sell_price_2() == 0);
            CHECK(g_reporter->get_trade_result()[1]->sell_price_1() == 0);
            CHECK(g_reporter->get_trade_result()[1]->buy_price_1() == 33.22);
            CHECK(g_reporter->get_trade_result()[1]->buy_price_2() == 22.33);
            CHECK(g_reporter->get_trade_result()[1]->buy_price_3() == 0);

            CHECK(g_reporter->get_trade_result()[2]->symbol() == "600875.SH");
            CHECK(g_reporter->get_trade_result()[2]->price() == 33.22);
            CHECK(g_reporter->get_trade_result()[2]->quantity() == 20);
            CHECK(g_reporter->get_trade_result()[2]->ask_unique_id() == 4);
            CHECK(g_reporter->get_trade_result()[2]->bid_unique_id() == 1);
            CHECK(g_reporter->get_trade_result()[2]->sell_price_3() == 0);
            CHECK(g_reporter->get_trade_result()[2]->sell_price_2() == 0);
            CHECK(g_reporter->get_trade_result()[2]->sell_price_1() == 0);
            CHECK(g_reporter->get_trade_result()[2]->buy_price_1() == 0);
            CHECK(g_reporter->get_trade_result()[2]->buy_price_2() == 0);
            CHECK(g_reporter->get_trade_result()[2]->buy_price_3() == 0);

            CHECK(g_reporter->get_trade_result()[3]->symbol() == "600875.SH");
            CHECK(g_reporter->get_trade_result()[3]->price() == 22.33);
            CHECK(g_reporter->get_trade_result()[3]->quantity() == 20);
            CHECK(g_reporter->get_trade_result()[3]->ask_unique_id() == 4);
            CHECK(g_reporter->get_trade_result()[3]->bid_unique_id() == 0);
            CHECK(g_reporter->get_trade_result()[3]->sell_price_3() == 0);
            CHECK(g_reporter->get_trade_result()[3]->sell_price_2() == 0);
            CHECK(g_reporter->get_trade_result()[3]->sell_price_1() == 0);
            CHECK(g_reporter->get_trade_result()[3]->buy_price_1() == 0);
            CHECK(g_reporter->get_trade_result()[3]->buy_price_2() == 0);
            CHECK(g_reporter->get_trade_result()[3]->buy_price_3() == 0);
        }

        SECTION("Buy after sell with step market price")
        {
            trade::booker::Booker booker({}, reporter());

            booker.add(OrderCreator::ororder_tick(0, LIMIT, "600875.SH", SELL, 22.22, 20));
            booker.add(OrderCreator::ororder_tick(1, LIMIT, "600875.SH", SELL, 22.33, 40));
            booker.add(OrderCreator::ororder_tick(2, LIMIT, "600875.SH", SELL, 33.22, 80));
            booker.add(OrderCreator::ororder_tick(3, MARKET, "600875.SH", BUY, 0, 100));
            booker.add(OrderCreator::ororder_tick(4, MARKET, "600875.SH", BUY, 0, 100));

            CHECK(g_reporter->get_trade_result().size() == 4);

            CHECK(g_reporter->get_trade_result()[0]->symbol() == "600875.SH");
            CHECK(g_reporter->get_trade_result()[0]->price() == 22.22);
            CHECK(g_reporter->get_trade_result()[0]->quantity() == 20);
            CHECK(g_reporter->get_trade_result()[0]->ask_unique_id() == 0);
            CHECK(g_reporter->get_trade_result()[0]->bid_unique_id() == 3);
            CHECK(g_reporter->get_trade_result()[0]->sell_price_3() == 0);
            CHECK(g_reporter->get_trade_result()[0]->sell_price_2() == 0);
            CHECK(g_reporter->get_trade_result()[0]->sell_price_1() == 33.22);
            CHECK(g_reporter->get_trade_result()[0]->buy_price_1() == 0);
            CHECK(g_reporter->get_trade_result()[0]->buy_price_2() == 0);
            CHECK(g_reporter->get_trade_result()[0]->buy_price_3() == 0);

            CHECK(g_reporter->get_trade_result()[1]->symbol() == "600875.SH");
            CHECK(g_reporter->get_trade_result()[1]->price() == 22.33);
            CHECK(g_reporter->get_trade_result()[1]->quantity() == 40);
            CHECK(g_reporter->get_trade_result()[1]->ask_unique_id() == 1);
            CHECK(g_reporter->get_trade_result()[1]->bid_unique_id() == 3);
            CHECK(g_reporter->get_trade_result()[1]->sell_price_3() == 0);
            CHECK(g_reporter->get_trade_result()[1]->sell_price_2() == 0);
            CHECK(g_reporter->get_trade_result()[1]->sell_price_1() == 33.22);
            CHECK(g_reporter->get_trade_result()[1]->buy_price_1() == 0);
            CHECK(g_reporter->get_trade_result()[1]->buy_price_2() == 0);
            CHECK(g_reporter->get_trade_result()[1]->buy_price_3() == 0);

            CHECK(g_reporter->get_trade_result()[2]->symbol() == "600875.SH");
            CHECK(g_reporter->get_trade_result()[2]->price() == 33.22);
            CHECK(g_reporter->get_trade_result()[2]->quantity() == 40);
            CHECK(g_reporter->get_trade_result()[2]->ask_unique_id() == 2);
            CHECK(g_reporter->get_trade_result()[2]->bid_unique_id() == 3);
            CHECK(g_reporter->get_trade_result()[2]->sell_price_3() == 0);
            CHECK(g_reporter->get_trade_result()[2]->sell_price_2() == 0);
            CHECK(g_reporter->get_trade_result()[2]->sell_price_1() == 33.22);
            CHECK(g_reporter->get_trade_result()[2]->buy_price_1() == 0);
            CHECK(g_reporter->get_trade_result()[2]->buy_price_2() == 0);
            CHECK(g_reporter->get_trade_result()[2]->buy_price_3() == 0);

            CHECK(g_reporter->get_trade_result()[3]->symbol() == "600875.SH");
            CHECK(g_reporter->get_trade_result()[3]->price() == 33.22);
            CHECK(g_reporter->get_trade_result()[3]->quantity() == 40);
            CHECK(g_reporter->get_trade_result()[3]->ask_unique_id() == 2);
            CHECK(g_reporter->get_trade_result()[3]->bid_unique_id() == 4);
            CHECK(g_reporter->get_trade_result()[3]->sell_price_3() == 0);
            CHECK(g_reporter->get_trade_result()[3]->sell_price_2() == 0);
            CHECK(g_reporter->get_trade_result()[3]->sell_price_1() == 0);
            CHECK(g_reporter->get_trade_result()[3]->buy_price_1() == 0);
            CHECK(g_reporter->get_trade_result()[3]->buy_price_2() == 0);
            CHECK(g_reporter->get_trade_result()[3]->buy_price_3() == 0);
        }
    }

    SECTION("Best price order matching")
    {
        SECTION("Sell after buy with best price")
        {
            trade::booker::Booker booker({}, reporter());

            booker.add(OrderCreator::ororder_tick(0, LIMIT, "600875.SH", BUY, 22.33, 20));
            booker.add(OrderCreator::ororder_tick(1, BEST_PRICE, "600875.SH", BUY, 0, 40));
            booker.add(OrderCreator::ororder_tick(2, LIMIT, "600875.SH", SELL, 22.33, 100));

            CHECK(g_reporter->get_trade_result().size() == 2);

            CHECK(g_reporter->get_trade_result()[0]->symbol() == "600875.SH");
            CHECK(g_reporter->get_trade_result()[0]->price() == 22.33);
            CHECK(g_reporter->get_trade_result()[0]->quantity() == 20);
            CHECK(g_reporter->get_trade_result()[0]->ask_unique_id() == 2);
            CHECK(g_reporter->get_trade_result()[0]->bid_unique_id() == 0);
            CHECK(g_reporter->get_trade_result()[0]->sell_price_3() == 0);
            CHECK(g_reporter->get_trade_result()[0]->sell_price_2() == 0);
            CHECK(g_reporter->get_trade_result()[0]->sell_price_1() == 22.33);
            CHECK(g_reporter->get_trade_result()[0]->buy_price_1() == 0);
            CHECK(g_reporter->get_trade_result()[0]->buy_price_2() == 0);
            CHECK(g_reporter->get_trade_result()[0]->buy_price_3() == 0);

            CHECK(g_reporter->get_trade_result()[1]->symbol() == "600875.SH");
            CHECK(g_reporter->get_trade_result()[1]->price() == 22.33);
            CHECK(g_reporter->get_trade_result()[1]->quantity() == 40);
            CHECK(g_reporter->get_trade_result()[1]->ask_unique_id() == 2);
            CHECK(g_reporter->get_trade_result()[1]->bid_unique_id() == 1);
            CHECK(g_reporter->get_trade_result()[1]->sell_price_3() == 0);
            CHECK(g_reporter->get_trade_result()[1]->sell_price_2() == 0);
            CHECK(g_reporter->get_trade_result()[1]->sell_price_1() == 22.33);
            CHECK(g_reporter->get_trade_result()[1]->buy_price_1() == 0);
            CHECK(g_reporter->get_trade_result()[1]->buy_price_2() == 0);
            CHECK(g_reporter->get_trade_result()[1]->buy_price_3() == 0);
        }

        SECTION("Buy after sell with best price")
        {
            trade::booker::Booker booker({}, reporter());

            booker.add(OrderCreator::ororder_tick(0, LIMIT, "600875.SH", SELL, 22.33, 20));
            booker.add(OrderCreator::ororder_tick(1, BEST_PRICE, "600875.SH", SELL, 0, 40));
            booker.add(OrderCreator::ororder_tick(2, LIMIT, "600875.SH", BUY, 22.33, 100));

            CHECK(g_reporter->get_trade_result().size() == 2);

            CHECK(g_reporter->get_trade_result()[0]->symbol() == "600875.SH");
            CHECK(g_reporter->get_trade_result()[0]->price() == 22.33);
            CHECK(g_reporter->get_trade_result()[0]->quantity() == 20);
            CHECK(g_reporter->get_trade_result()[0]->ask_unique_id() == 0);
            CHECK(g_reporter->get_trade_result()[0]->bid_unique_id() == 2);
            CHECK(g_reporter->get_trade_result()[0]->sell_price_3() == 0);
            CHECK(g_reporter->get_trade_result()[0]->sell_price_2() == 0);
            CHECK(g_reporter->get_trade_result()[0]->sell_price_1() == 0);
            CHECK(g_reporter->get_trade_result()[0]->buy_price_1() == 22.33);
            CHECK(g_reporter->get_trade_result()[0]->buy_price_2() == 0);
            CHECK(g_reporter->get_trade_result()[0]->buy_price_3() == 0);

            CHECK(g_reporter->get_trade_result()[1]->symbol() == "600875.SH");
            CHECK(g_reporter->get_trade_result()[1]->price() == 22.33);
            CHECK(g_reporter->get_trade_result()[1]->quantity() == 40);
            CHECK(g_reporter->get_trade_result()[1]->ask_unique_id() == 1);
            CHECK(g_reporter->get_trade_result()[1]->bid_unique_id() == 2);
            CHECK(g_reporter->get_trade_result()[1]->sell_price_3() == 0);
            CHECK(g_reporter->get_trade_result()[1]->sell_price_2() == 0);
            CHECK(g_reporter->get_trade_result()[1]->sell_price_1() == 0);
            CHECK(g_reporter->get_trade_result()[1]->buy_price_1() == 22.33);
            CHECK(g_reporter->get_trade_result()[1]->buy_price_2() == 0);
            CHECK(g_reporter->get_trade_result()[1]->buy_price_3() == 0);
        }
    }

    SECTION("Call auction stage")
    {
        SECTION("No trade after call auction")
        {
            trade::booker::Booker booker({}, reporter());

            booker.add(OrderCreator::ororder_tick(0, LIMIT, "600875.SH", SELL, 22.33, 20, 91500000));
            booker.add(OrderCreator::ororder_tick(1, LIMIT, "600875.SH", SELL, 33.22, 40, 91500000));
            booker.add(OrderCreator::ororder_tick(2, LIMIT, "600875.SH", SELL, 33.33, 80, 91500000));
            booker.add(OrderCreator::ororder_tick(3, LIMIT, "600875.SH", BUY, 22.22, 100, 91500000));
            booker.add(OrderCreator::ororder_tick(4, LIMIT, "600875.SH", BUY, 22.33, 100, 91500000));
            booker.add(OrderCreator::ororder_tick(5, LIMIT, "600875.SH", BUY, 33.22, 100, 91500000));

            booker.trade(OrderCreator::trade_tick(1, 5, "600875.SH", 33.22, 40, 92500000));
            booker.trade(OrderCreator::trade_tick(0, 5, "600875.SH", 33.22, 20, 92500000));

            booker.switch_to_continuous_stage();

            CHECK(g_reporter->get_trade_result().size() == 2);

            CHECK(g_reporter->get_trade_result()[0]->symbol() == "600875.SH");
            CHECK(g_reporter->get_trade_result()[0]->price() == 33.22);
            CHECK(g_reporter->get_trade_result()[0]->quantity() == 40);
            CHECK(g_reporter->get_trade_result()[0]->ask_unique_id() == 1);
            CHECK(g_reporter->get_trade_result()[0]->bid_unique_id() == 5);
            CHECK(g_reporter->get_trade_result()[0]->sell_price_3() == 0);
            CHECK(g_reporter->get_trade_result()[0]->sell_price_2() == 0);
            CHECK(g_reporter->get_trade_result()[0]->sell_price_1() == 0);
            CHECK(g_reporter->get_trade_result()[0]->buy_price_1() == 0);
            CHECK(g_reporter->get_trade_result()[0]->buy_price_2() == 0);
            CHECK(g_reporter->get_trade_result()[0]->buy_price_3() == 0);
        }

        SECTION("Continuous trade after call auction")
        {
            trade::booker::Booker booker({}, reporter());

            booker.add(OrderCreator::ororder_tick(0, LIMIT, "600875.SH", SELL, 22.33, 20, 91500000));
            booker.add(OrderCreator::ororder_tick(1, LIMIT, "600875.SH", SELL, 33.22, 40, 91500000));
            booker.add(OrderCreator::ororder_tick(2, LIMIT, "600875.SH", SELL, 33.33, 80, 91500000));
            booker.add(OrderCreator::ororder_tick(3, LIMIT, "600875.SH", BUY, 22.22, 100, 91500000));
            booker.add(OrderCreator::ororder_tick(4, LIMIT, "600875.SH", BUY, 22.33, 100, 91500000));
            booker.add(OrderCreator::ororder_tick(5, LIMIT, "600875.SH", BUY, 33.22, 100, 91500000));

            booker.trade(OrderCreator::trade_tick(1, 5, "600875.SH", 33.22, 40, 92500000));
            booker.trade(OrderCreator::trade_tick(0, 5, "600875.SH", 33.22, 20, 92500000));

            booker.switch_to_continuous_stage();

            booker.add(OrderCreator::ororder_tick(6, MARKET, "600875.SH", BUY, 0, 80, 93000000));
            booker.add(OrderCreator::ororder_tick(7, MARKET, "600875.SH", SELL, 0, 240, 93000000));

            CHECK(g_reporter->get_trade_result().size() == 6);

            CHECK(g_reporter->get_trade_result()[0]->symbol() == "600875.SH");
            CHECK(g_reporter->get_trade_result()[0]->price() == 33.22);
            CHECK(g_reporter->get_trade_result()[0]->quantity() == 40);
            CHECK(g_reporter->get_trade_result()[0]->ask_unique_id() == 1);
            CHECK(g_reporter->get_trade_result()[0]->bid_unique_id() == 5);
            CHECK(g_reporter->get_trade_result()[0]->sell_price_3() == 0);
            CHECK(g_reporter->get_trade_result()[0]->sell_price_2() == 0);
            CHECK(g_reporter->get_trade_result()[0]->sell_price_1() == 0);
            CHECK(g_reporter->get_trade_result()[0]->buy_price_1() == 0);
            CHECK(g_reporter->get_trade_result()[0]->buy_price_2() == 0);
            CHECK(g_reporter->get_trade_result()[0]->buy_price_3() == 0);

            CHECK(g_reporter->get_trade_result()[1]->symbol() == "600875.SH");
            CHECK(g_reporter->get_trade_result()[1]->price() == 33.22);
            CHECK(g_reporter->get_trade_result()[1]->quantity() == 20);
            CHECK(g_reporter->get_trade_result()[1]->ask_unique_id() == 0);
            CHECK(g_reporter->get_trade_result()[1]->bid_unique_id() == 5);
            CHECK(g_reporter->get_trade_result()[1]->sell_price_3() == 0);
            CHECK(g_reporter->get_trade_result()[1]->sell_price_2() == 0);
            CHECK(g_reporter->get_trade_result()[1]->sell_price_1() == 0);
            CHECK(g_reporter->get_trade_result()[1]->buy_price_1() == 0);
            CHECK(g_reporter->get_trade_result()[1]->buy_price_2() == 0);
            CHECK(g_reporter->get_trade_result()[1]->buy_price_3() == 0);

            CHECK(g_reporter->get_trade_result()[2]->symbol() == "600875.SH");
            CHECK(g_reporter->get_trade_result()[2]->price() == 33.33);
            CHECK(g_reporter->get_trade_result()[2]->quantity() == 80);
            CHECK(g_reporter->get_trade_result()[2]->ask_unique_id() == 2);
            CHECK(g_reporter->get_trade_result()[2]->bid_unique_id() == 6);
            CHECK(g_reporter->get_trade_result()[2]->sell_price_3() == 0);
            CHECK(g_reporter->get_trade_result()[2]->sell_price_2() == 0);
            CHECK(g_reporter->get_trade_result()[2]->sell_price_1() == 0);
            CHECK(g_reporter->get_trade_result()[2]->buy_price_1() == 33.22);
            CHECK(g_reporter->get_trade_result()[2]->buy_price_2() == 22.33);
            CHECK(g_reporter->get_trade_result()[2]->buy_price_3() == 22.22);

            CHECK(g_reporter->get_trade_result()[3]->symbol() == "600875.SH");
            CHECK(g_reporter->get_trade_result()[3]->price() == 33.22);
            CHECK(g_reporter->get_trade_result()[3]->quantity() == 40);
            CHECK(g_reporter->get_trade_result()[3]->ask_unique_id() == 7);
            CHECK(g_reporter->get_trade_result()[3]->bid_unique_id() == 5);
            CHECK(g_reporter->get_trade_result()[3]->sell_price_3() == 0);
            CHECK(g_reporter->get_trade_result()[3]->sell_price_2() == 0);
            CHECK(g_reporter->get_trade_result()[3]->sell_price_1() == 0);
            CHECK(g_reporter->get_trade_result()[3]->buy_price_1() == 0);
            CHECK(g_reporter->get_trade_result()[3]->buy_price_2() == 0);
            CHECK(g_reporter->get_trade_result()[3]->buy_price_3() == 0);

            CHECK(g_reporter->get_trade_result()[4]->symbol() == "600875.SH");
            CHECK(g_reporter->get_trade_result()[4]->price() == 22.33);
            CHECK(g_reporter->get_trade_result()[4]->quantity() == 100);
            CHECK(g_reporter->get_trade_result()[4]->ask_unique_id() == 7);
            CHECK(g_reporter->get_trade_result()[4]->bid_unique_id() == 4);
            CHECK(g_reporter->get_trade_result()[4]->sell_price_3() == 0);
            CHECK(g_reporter->get_trade_result()[4]->sell_price_2() == 0);
            CHECK(g_reporter->get_trade_result()[4]->sell_price_1() == 0);
            CHECK(g_reporter->get_trade_result()[4]->buy_price_1() == 0);
            CHECK(g_reporter->get_trade_result()[4]->buy_price_2() == 0);
            CHECK(g_reporter->get_trade_result()[4]->buy_price_3() == 0);

            CHECK(g_reporter->get_trade_result()[5]->symbol() == "600875.SH");
            CHECK(g_reporter->get_trade_result()[5]->price() == 22.22);
            CHECK(g_reporter->get_trade_result()[5]->quantity() == 100);
            CHECK(g_reporter->get_trade_result()[5]->ask_unique_id() == 7);
            CHECK(g_reporter->get_trade_result()[5]->bid_unique_id() == 3);
            CHECK(g_reporter->get_trade_result()[5]->sell_price_3() == 0);
            CHECK(g_reporter->get_trade_result()[5]->sell_price_2() == 0);
            CHECK(g_reporter->get_trade_result()[5]->sell_price_1() == 0);
            CHECK(g_reporter->get_trade_result()[5]->buy_price_1() == 0);
            CHECK(g_reporter->get_trade_result()[5]->buy_price_2() == 0);
            CHECK(g_reporter->get_trade_result()[5]->buy_price_3() == 0);
        }
    }
}
