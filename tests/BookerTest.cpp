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
        m_trade_result.str("");
        m_market_price_result.str("");
    }

public:
    [[nodiscard]] std::string get_trade_result() const
    {
        return m_trade_result.str();
    }

    [[nodiscard]] std::string get_market_price_result() const
    {
        return m_market_price_result.str();
    }

public:
    void md_trade_generated(const std::shared_ptr<trade::types::MdTrade> md_trade) override
    {
        m_trade_result << fmt::format("{}:{:05.2f}:{:04}\n", md_trade->symbol(), md_trade->price(), md_trade->quantity());
    }

    void market_price(std::string symbol, double price) override
    {
        m_market_price_result << fmt::format("{}:{:05.2f}\n", symbol, price);
    }

private:
    std::ostringstream m_trade_result;
    std::ostringstream m_market_price_result;
};

const auto g_reporter = std::make_shared<SeqChecker>();

/// For reusing the same reporter.
const auto reporter = [] {g_reporter->reset(); return g_reporter; };

TEST_CASE("Limit order matching with 1:1 matching", "[Booker]")
{
    SECTION("Sell after buy with same price")
    {
        trade::booker::Booker booker({}, reporter());

        booker.add(OrderCreator::ororder_tick(0, LIMIT, "600875.SH", BUY, 22.33, 100));
        booker.add(OrderCreator::ororder_tick(1, LIMIT, "600875.SH", SELL, 22.33, 100));

        CHECK(g_reporter->get_trade_result() == "600875.SH:22.33:0100\n");
        CHECK(g_reporter->get_market_price_result() == "600875.SH:22.33\n");
    }

    SECTION("Buy after sell with same price")
    {
        trade::booker::Booker booker({}, reporter());

        booker.add(OrderCreator::ororder_tick(0, LIMIT, "600875.SH", SELL, 22.33, 100));
        booker.add(OrderCreator::ororder_tick(1, LIMIT, "600875.SH", BUY, 22.33, 100));

        CHECK(g_reporter->get_trade_result() == "600875.SH:22.33:0100\n");
        CHECK(g_reporter->get_market_price_result() == "600875.SH:22.33\n");
    }

    SECTION("Sell after buy with lower price")
    {
        trade::booker::Booker booker({}, reporter());

        booker.add(OrderCreator::ororder_tick(0, LIMIT, "600875.SH", BUY, 33.22, 100));
        booker.add(OrderCreator::ororder_tick(1, LIMIT, "600875.SH", SELL, 22.33, 100));

        CHECK(g_reporter->get_trade_result() == "600875.SH:33.22:0100\n");
        CHECK(g_reporter->get_market_price_result() == "600875.SH:33.22\n");
    }

    SECTION("Buy after sell with higher price")
    {
        trade::booker::Booker booker({}, reporter());

        booker.add(OrderCreator::ororder_tick(0, LIMIT, "600875.SH", SELL, 22.33, 100));
        booker.add(OrderCreator::ororder_tick(1, LIMIT, "600875.SH", BUY, 33.22, 100));

        CHECK(g_reporter->get_trade_result() == "600875.SH:22.33:0100\n");
        CHECK(g_reporter->get_market_price_result() == "600875.SH:22.33\n");
    }

    SECTION("Sell after buy with higher price")
    {
        trade::booker::Booker booker({}, reporter());

        booker.add(OrderCreator::ororder_tick(0, LIMIT, "600875.SH", BUY, 22.33, 100));
        booker.add(OrderCreator::ororder_tick(1, LIMIT, "600875.SH", SELL, 33.22, 100));

        CHECK(g_reporter->get_trade_result().empty());
        CHECK(g_reporter->get_market_price_result().empty());
    }

    SECTION("Buy after sell with lower price")
    {
        trade::booker::Booker booker({}, reporter());

        booker.add(OrderCreator::ororder_tick(0, LIMIT, "600875.SH", SELL, 33.22, 100));
        booker.add(OrderCreator::ororder_tick(1, LIMIT, "600875.SH", BUY, 22.33, 100));

        CHECK(g_reporter->get_trade_result().empty());
        CHECK(g_reporter->get_market_price_result().empty());
    }
}

TEST_CASE("Limit order matching with M:N matching", "[Booker]")
{
    SECTION("Buy after sell with same price")
    {
        trade::booker::Booker booker({}, reporter());

        booker.add(OrderCreator::ororder_tick(0, LIMIT, "600875.SH", SELL, 22.33, 20));
        booker.add(OrderCreator::ororder_tick(1, LIMIT, "600875.SH", SELL, 22.33, 40));
        booker.add(OrderCreator::ororder_tick(2, LIMIT, "600875.SH", SELL, 22.33, 80));
        booker.add(OrderCreator::ororder_tick(3, LIMIT, "600875.SH", BUY, 22.33, 100));
        booker.add(OrderCreator::ororder_tick(4, LIMIT, "600875.SH", BUY, 22.33, 100));

        CHECK(g_reporter->get_trade_result() == "600875.SH:22.33:0020\n"
                                                "600875.SH:22.33:0040\n"
                                                "600875.SH:22.33:0040\n"
                                                "600875.SH:22.33:0040\n");
        CHECK(g_reporter->get_market_price_result() == "600875.SH:22.33\n"
                                                       "600875.SH:22.33\n"
                                                       "600875.SH:22.33\n"
                                                       "600875.SH:22.33\n");
    }

    SECTION("Buy after sell with same price")
    {
        trade::booker::Booker booker({}, reporter());

        booker.add(OrderCreator::ororder_tick(0, LIMIT, "600875.SH", SELL, 22.33, 20));
        booker.add(OrderCreator::ororder_tick(1, LIMIT, "600875.SH", SELL, 22.33, 40));
        booker.add(OrderCreator::ororder_tick(2, LIMIT, "600875.SH", SELL, 22.33, 80));
        booker.add(OrderCreator::ororder_tick(3, LIMIT, "600875.SH", BUY, 22.33, 100));
        booker.add(OrderCreator::ororder_tick(4, LIMIT, "600875.SH", BUY, 22.33, 100));

        CHECK(g_reporter->get_trade_result() == "600875.SH:22.33:0020\n"
                                                "600875.SH:22.33:0040\n"
                                                "600875.SH:22.33:0040\n"
                                                "600875.SH:22.33:0040\n");
        CHECK(g_reporter->get_market_price_result() == "600875.SH:22.33\n"
                                                       "600875.SH:22.33\n"
                                                       "600875.SH:22.33\n"
                                                       "600875.SH:22.33\n");
    }

    SECTION("Sell after buy with lower price")
    {
        trade::booker::Booker booker({}, reporter());

        booker.add(OrderCreator::ororder_tick(0, LIMIT, "600875.SH", BUY, 33.22, 20));
        booker.add(OrderCreator::ororder_tick(1, LIMIT, "600875.SH", BUY, 33.22, 40));
        booker.add(OrderCreator::ororder_tick(2, LIMIT, "600875.SH", BUY, 33.22, 80));
        booker.add(OrderCreator::ororder_tick(3, LIMIT, "600875.SH", SELL, 22.33, 100));
        booker.add(OrderCreator::ororder_tick(4, LIMIT, "600875.SH", SELL, 22.33, 100));

        CHECK(g_reporter->get_trade_result() == "600875.SH:33.22:0020\n"
                                                "600875.SH:33.22:0040\n"
                                                "600875.SH:33.22:0040\n"
                                                "600875.SH:33.22:0040\n");
        CHECK(g_reporter->get_market_price_result() == "600875.SH:33.22\n"
                                                       "600875.SH:33.22\n"
                                                       "600875.SH:33.22\n"
                                                       "600875.SH:33.22\n");
    }

    SECTION("Buy after sell with higher price")
    {
        trade::booker::Booker booker({}, reporter());

        booker.add(OrderCreator::ororder_tick(0, LIMIT, "600875.SH", SELL, 22.33, 20));
        booker.add(OrderCreator::ororder_tick(1, LIMIT, "600875.SH", SELL, 22.33, 40));
        booker.add(OrderCreator::ororder_tick(2, LIMIT, "600875.SH", SELL, 22.33, 80));
        booker.add(OrderCreator::ororder_tick(3, LIMIT, "600875.SH", BUY, 33.22, 100));
        booker.add(OrderCreator::ororder_tick(4, LIMIT, "600875.SH", BUY, 33.22, 100));

        CHECK(g_reporter->get_trade_result() == "600875.SH:22.33:0020\n"
                                                "600875.SH:22.33:0040\n"
                                                "600875.SH:22.33:0040\n"
                                                "600875.SH:22.33:0040\n");
        CHECK(g_reporter->get_market_price_result() == "600875.SH:22.33\n"
                                                       "600875.SH:22.33\n"
                                                       "600875.SH:22.33\n"
                                                       "600875.SH:22.33\n");
    }

    SECTION("Sell after buy with step price")
    {
        trade::booker::Booker booker({}, reporter());

        booker.add(OrderCreator::ororder_tick(0, LIMIT, "600875.SH", BUY, 22.33, 20));
        booker.add(OrderCreator::ororder_tick(1, LIMIT, "600875.SH", BUY, 33.22, 40));
        booker.add(OrderCreator::ororder_tick(2, LIMIT, "600875.SH", BUY, 33.33, 80));
        booker.add(OrderCreator::ororder_tick(3, LIMIT, "600875.SH", SELL, 33.22, 100));
        booker.add(OrderCreator::ororder_tick(4, LIMIT, "600875.SH", SELL, 33.22, 100));

        CHECK(g_reporter->get_trade_result() == "600875.SH:33.33:0080\n"
                                                "600875.SH:33.22:0020\n"
                                                "600875.SH:33.22:0020\n");
        CHECK(g_reporter->get_market_price_result() == "600875.SH:33.33\n"
                                                       "600875.SH:33.22\n"
                                                       "600875.SH:33.22\n");
    }

    SECTION("Buy after sell with step price")
    {
        trade::booker::Booker booker({}, reporter());

        booker.add(OrderCreator::ororder_tick(0, LIMIT, "600875.SH", SELL, 22.22, 20));
        booker.add(OrderCreator::ororder_tick(1, LIMIT, "600875.SH", SELL, 22.33, 40));
        booker.add(OrderCreator::ororder_tick(2, LIMIT, "600875.SH", SELL, 33.22, 80));
        booker.add(OrderCreator::ororder_tick(3, LIMIT, "600875.SH", BUY, 22.33, 100));
        booker.add(OrderCreator::ororder_tick(4, LIMIT, "600875.SH", BUY, 22.33, 100));

        CHECK(g_reporter->get_trade_result() == "600875.SH:22.22:0020\n"
                                                "600875.SH:22.33:0040\n");
        CHECK(g_reporter->get_market_price_result() == "600875.SH:22.22\n"
                                                       "600875.SH:22.33\n");
    }
}

TEST_CASE("Limit order with cancel", "[Booker]")
{
    SECTION("Limit order with cancel before fill")
    {
        trade::booker::Booker booker({}, reporter());

        booker.add(OrderCreator::ororder_tick(0, LIMIT, "600875.SH", BUY, 22.33, 50));
        booker.add(OrderCreator::ororder_tick(0, CANCEL, "600875.SH", BUY, 22.33, 50));
        booker.add(OrderCreator::ororder_tick(1, LIMIT, "600875.SH", SELL, 22.33, 100));

        CHECK(g_reporter->get_trade_result().empty());
        CHECK(g_reporter->get_market_price_result().empty());
    }

    SECTION("Limit order with cancel after partial fill")
    {
        trade::booker::Booker booker({}, reporter());

        booker.add(OrderCreator::ororder_tick(0, LIMIT, "600875.SH", BUY, 22.33, 100));
        booker.add(OrderCreator::ororder_tick(1, LIMIT, "600875.SH", SELL, 22.33, 50));
        booker.add(OrderCreator::ororder_tick(0, CANCEL, "600875.SH", BUY, 22.33, 100));
        booker.add(OrderCreator::ororder_tick(2, LIMIT, "600875.SH", SELL, 22.33, 50));

        CHECK(g_reporter->get_trade_result() == "600875.SH:22.33:0050\n");
        CHECK(g_reporter->get_market_price_result() == "600875.SH:22.33\n");
    }

    SECTION("Limit order with cancel after full fill")
    {
        trade::booker::Booker booker({}, reporter());

        booker.add(OrderCreator::ororder_tick(0, LIMIT, "600875.SH", BUY, 22.33, 100));
        booker.add(OrderCreator::ororder_tick(1, LIMIT, "600875.SH", SELL, 22.33, 100));
        booker.add(OrderCreator::ororder_tick(0, CANCEL, "600875.SH", BUY, 22.33, 100));

        CHECK(g_reporter->get_trade_result() == "600875.SH:22.33:0100\n");
        CHECK(g_reporter->get_market_price_result() == "600875.SH:22.33\n");
    }
}

TEST_CASE("Market order matching with 1:1 matching", "[Booker]")
{
    SECTION("Sell after buy with market price")
    {
        trade::booker::Booker booker({}, reporter());

        booker.add(OrderCreator::ororder_tick(0, LIMIT, "600875.SH", BUY, 22.33, 100));
        booker.add(OrderCreator::ororder_tick(1, LIMIT, "600875.SH", BUY, 33.22, 100));
        booker.add(OrderCreator::ororder_tick(2, MARKET, "600875.SH", SELL, 0, 100));

        CHECK(g_reporter->get_trade_result() == "600875.SH:33.22:0100\n");
        CHECK(g_reporter->get_market_price_result() == "600875.SH:33.22\n");
    }

    SECTION("Buy after sell with market price")
    {
        trade::booker::Booker booker({}, reporter());

        booker.add(OrderCreator::ororder_tick(0, LIMIT, "600875.SH", SELL, 22.33, 100));
        booker.add(OrderCreator::ororder_tick(1, LIMIT, "600875.SH", SELL, 33.22, 100));
        booker.add(OrderCreator::ororder_tick(2, MARKET, "600875.SH", BUY, 0, 100));

        CHECK(g_reporter->get_trade_result() == "600875.SH:22.33:0100\n");
        CHECK(g_reporter->get_market_price_result() == "600875.SH:22.33\n");
    }
}

TEST_CASE("Market order matching with M:N matching", "[Booker]")
{
    SECTION("Buy after sell with market price")
    {
        trade::booker::Booker booker({}, reporter());

        booker.add(OrderCreator::ororder_tick(0, LIMIT, "600875.SH", SELL, 22.33, 20));
        booker.add(OrderCreator::ororder_tick(1, LIMIT, "600875.SH", SELL, 22.33, 40));
        booker.add(OrderCreator::ororder_tick(2, LIMIT, "600875.SH", SELL, 22.33, 80));
        booker.add(OrderCreator::ororder_tick(3, MARKET, "600875.SH", BUY, 0, 100));
        booker.add(OrderCreator::ororder_tick(4, MARKET, "600875.SH", BUY, 0, 100));

        CHECK(g_reporter->get_trade_result() == "600875.SH:22.33:0020\n"
                                                "600875.SH:22.33:0040\n"
                                                "600875.SH:22.33:0040\n"
                                                "600875.SH:22.33:0040\n");
        CHECK(g_reporter->get_market_price_result() == "600875.SH:22.33\n"
                                                       "600875.SH:22.33\n"
                                                       "600875.SH:22.33\n"
                                                       "600875.SH:22.33\n");
    }

    SECTION("Buy after sell with market price")
    {
        trade::booker::Booker booker({}, reporter());

        booker.add(OrderCreator::ororder_tick(0, LIMIT, "600875.SH", SELL, 22.33, 20));
        booker.add(OrderCreator::ororder_tick(1, LIMIT, "600875.SH", SELL, 22.33, 40));
        booker.add(OrderCreator::ororder_tick(2, LIMIT, "600875.SH", SELL, 22.33, 80));
        booker.add(OrderCreator::ororder_tick(3, MARKET, "600875.SH", BUY, 0, 100));
        booker.add(OrderCreator::ororder_tick(4, MARKET, "600875.SH", BUY, 0, 100));

        CHECK(g_reporter->get_trade_result() == "600875.SH:22.33:0020\n"
                                                "600875.SH:22.33:0040\n"
                                                "600875.SH:22.33:0040\n"
                                                "600875.SH:22.33:0040\n");
        CHECK(g_reporter->get_market_price_result() == "600875.SH:22.33\n"
                                                       "600875.SH:22.33\n"
                                                       "600875.SH:22.33\n"
                                                       "600875.SH:22.33\n");
    }

    SECTION("Sell after buy with step market price")
    {
        trade::booker::Booker booker({}, reporter());

        booker.add(OrderCreator::ororder_tick(0, LIMIT, "600875.SH", BUY, 22.33, 20));
        booker.add(OrderCreator::ororder_tick(1, LIMIT, "600875.SH", BUY, 33.22, 40));
        booker.add(OrderCreator::ororder_tick(2, LIMIT, "600875.SH", BUY, 33.33, 80));
        booker.add(OrderCreator::ororder_tick(3, MARKET, "600875.SH", SELL, 0, 100));
        booker.add(OrderCreator::ororder_tick(4, MARKET, "600875.SH", SELL, 0, 100));

        CHECK(g_reporter->get_trade_result() == "600875.SH:33.33:0080\n"
                                                "600875.SH:33.22:0020\n"
                                                "600875.SH:33.22:0020\n"
                                                "600875.SH:22.33:0020\n");
        CHECK(g_reporter->get_market_price_result() == "600875.SH:33.33\n"
                                                       "600875.SH:33.22\n"
                                                       "600875.SH:33.22\n"
                                                       "600875.SH:22.33\n");
    }

    SECTION("Buy after sell with step market price")
    {
        trade::booker::Booker booker({}, reporter());

        booker.add(OrderCreator::ororder_tick(0, LIMIT, "600875.SH", SELL, 22.22, 20));
        booker.add(OrderCreator::ororder_tick(1, LIMIT, "600875.SH", SELL, 22.33, 40));
        booker.add(OrderCreator::ororder_tick(2, LIMIT, "600875.SH", SELL, 33.22, 80));
        booker.add(OrderCreator::ororder_tick(3, MARKET, "600875.SH", BUY, 0, 100));
        booker.add(OrderCreator::ororder_tick(4, MARKET, "600875.SH", BUY, 0, 100));

        CHECK(g_reporter->get_trade_result() == "600875.SH:22.22:0020\n"
                                                "600875.SH:22.33:0040\n"
                                                "600875.SH:33.22:0040\n"
                                                "600875.SH:33.22:0040\n");
        CHECK(g_reporter->get_market_price_result() == "600875.SH:22.22\n"
                                                       "600875.SH:22.33\n"
                                                       "600875.SH:33.22\n"
                                                       "600875.SH:33.22\n");
    }
}

TEST_CASE("Best price order matching", "[Booker]")
{
    SECTION("Sell after buy with best price")
    {
        trade::booker::Booker booker({}, reporter());

        booker.add(OrderCreator::ororder_tick(0, LIMIT, "600875.SH", BUY, 22.33, 20));
        booker.add(OrderCreator::ororder_tick(1, BEST_PRICE, "600875.SH", BUY, 0, 40));
        booker.add(OrderCreator::ororder_tick(2, LIMIT, "600875.SH", SELL, 22.33, 100));

        CHECK(g_reporter->get_trade_result() == "600875.SH:22.33:0020\n"
                                                "600875.SH:22.33:0040\n");
        CHECK(g_reporter->get_market_price_result() == "600875.SH:22.33\n"
                                                       "600875.SH:22.33\n");
    }

    SECTION("Buy after sell with best price")
    {
        trade::booker::Booker booker({}, reporter());

        booker.add(OrderCreator::ororder_tick(0, LIMIT, "600875.SH", SELL, 22.33, 20));
        booker.add(OrderCreator::ororder_tick(1, BEST_PRICE, "600875.SH", SELL, 0, 40));
        booker.add(OrderCreator::ororder_tick(2, LIMIT, "600875.SH", BUY, 22.33, 100));

        CHECK(g_reporter->get_trade_result() == "600875.SH:22.33:0020\n"
                                                "600875.SH:22.33:0040\n");
        CHECK(g_reporter->get_market_price_result() == "600875.SH:22.33\n"
                                                       "600875.SH:22.33\n");
    }
}

TEST_CASE("Limit unordered order matching with M:N matching", "[Booker]")
{
    SECTION("Buy after sell with same price")
    {
        trade::booker::Booker booker({}, reporter());

        booker.add(OrderCreator::ororder_tick(0, LIMIT, "600875.SH", SELL, 22.33, 20));
        booker.add(OrderCreator::ororder_tick(3, LIMIT, "600875.SH", BUY, 22.33, 100));
        booker.add(OrderCreator::ororder_tick(2, LIMIT, "600875.SH", SELL, 22.33, 80));
        booker.add(OrderCreator::ororder_tick(1, LIMIT, "600875.SH", SELL, 22.33, 40));
        booker.add(OrderCreator::ororder_tick(4, LIMIT, "600875.SH", BUY, 22.33, 100));

        CHECK(g_reporter->get_trade_result() == "600875.SH:22.33:0020\n"
                                                "600875.SH:22.33:0040\n"
                                                "600875.SH:22.33:0040\n"
                                                "600875.SH:22.33:0040\n");
        CHECK(g_reporter->get_market_price_result() == "600875.SH:22.33\n"
                                                       "600875.SH:22.33\n"
                                                       "600875.SH:22.33\n"
                                                       "600875.SH:22.33\n");
    }

    SECTION("Buy after sell with same price")
    {
        trade::booker::Booker booker({}, reporter());

        booker.add(OrderCreator::ororder_tick(0, LIMIT, "600875.SH", SELL, 22.33, 20));
        booker.add(OrderCreator::ororder_tick(3, LIMIT, "600875.SH", BUY, 22.33, 100));
        booker.add(OrderCreator::ororder_tick(2, LIMIT, "600875.SH", SELL, 22.33, 80));
        booker.add(OrderCreator::ororder_tick(1, LIMIT, "600875.SH", SELL, 22.33, 40));
        booker.add(OrderCreator::ororder_tick(4, LIMIT, "600875.SH", BUY, 22.33, 100));

        CHECK(g_reporter->get_trade_result() == "600875.SH:22.33:0020\n"
                                                "600875.SH:22.33:0040\n"
                                                "600875.SH:22.33:0040\n"
                                                "600875.SH:22.33:0040\n");
        CHECK(g_reporter->get_market_price_result() == "600875.SH:22.33\n"
                                                       "600875.SH:22.33\n"
                                                       "600875.SH:22.33\n"
                                                       "600875.SH:22.33\n");
    }

    SECTION("Sell after buy with lower price")
    {
        trade::booker::Booker booker({}, reporter());

        booker.add(OrderCreator::ororder_tick(0, LIMIT, "600875.SH", BUY, 33.22, 20));
        booker.add(OrderCreator::ororder_tick(3, LIMIT, "600875.SH", SELL, 22.33, 100));
        booker.add(OrderCreator::ororder_tick(2, LIMIT, "600875.SH", BUY, 33.22, 80));
        booker.add(OrderCreator::ororder_tick(1, LIMIT, "600875.SH", BUY, 33.22, 40));
        booker.add(OrderCreator::ororder_tick(4, LIMIT, "600875.SH", SELL, 22.33, 100));

        CHECK(g_reporter->get_trade_result() == "600875.SH:33.22:0020\n"
                                                "600875.SH:33.22:0040\n"
                                                "600875.SH:33.22:0040\n"
                                                "600875.SH:33.22:0040\n");
        CHECK(g_reporter->get_market_price_result() == "600875.SH:33.22\n"
                                                       "600875.SH:33.22\n"
                                                       "600875.SH:33.22\n"
                                                       "600875.SH:33.22\n");
    }

    SECTION("Buy after sell with higher price")
    {
        trade::booker::Booker booker({}, reporter());

        booker.add(OrderCreator::ororder_tick(0, LIMIT, "600875.SH", SELL, 22.33, 20));
        booker.add(OrderCreator::ororder_tick(3, LIMIT, "600875.SH", BUY, 33.22, 100));
        booker.add(OrderCreator::ororder_tick(2, LIMIT, "600875.SH", SELL, 22.33, 80));
        booker.add(OrderCreator::ororder_tick(1, LIMIT, "600875.SH", SELL, 22.33, 40));
        booker.add(OrderCreator::ororder_tick(4, LIMIT, "600875.SH", BUY, 33.22, 100));

        CHECK(g_reporter->get_trade_result() == "600875.SH:22.33:0020\n"
                                                "600875.SH:22.33:0040\n"
                                                "600875.SH:22.33:0040\n"
                                                "600875.SH:22.33:0040\n");
        CHECK(g_reporter->get_market_price_result() == "600875.SH:22.33\n"
                                                       "600875.SH:22.33\n"
                                                       "600875.SH:22.33\n"
                                                       "600875.SH:22.33\n");
    }

    SECTION("Sell after buy with step price")
    {
        trade::booker::Booker booker({}, reporter());

        booker.add(OrderCreator::ororder_tick(0, LIMIT, "600875.SH", BUY, 22.33, 20));
        booker.add(OrderCreator::ororder_tick(3, LIMIT, "600875.SH", SELL, 33.22, 100));
        booker.add(OrderCreator::ororder_tick(2, LIMIT, "600875.SH", BUY, 33.33, 80));
        booker.add(OrderCreator::ororder_tick(1, LIMIT, "600875.SH", BUY, 33.22, 40));
        booker.add(OrderCreator::ororder_tick(4, LIMIT, "600875.SH", SELL, 33.22, 100));

        CHECK(g_reporter->get_trade_result() == "600875.SH:33.33:0080\n"
                                                "600875.SH:33.22:0020\n"
                                                "600875.SH:33.22:0020\n");
        CHECK(g_reporter->get_market_price_result() == "600875.SH:33.33\n"
                                                       "600875.SH:33.22\n"
                                                       "600875.SH:33.22\n");
    }

    SECTION("Buy after sell with step price")
    {
        trade::booker::Booker booker({}, reporter());

        booker.add(OrderCreator::ororder_tick(0, LIMIT, "600875.SH", SELL, 22.22, 20));
        booker.add(OrderCreator::ororder_tick(3, LIMIT, "600875.SH", BUY, 22.33, 100));
        booker.add(OrderCreator::ororder_tick(2, LIMIT, "600875.SH", SELL, 33.22, 80));
        booker.add(OrderCreator::ororder_tick(1, LIMIT, "600875.SH", SELL, 22.33, 40));
        booker.add(OrderCreator::ororder_tick(4, LIMIT, "600875.SH", BUY, 22.33, 100));

        CHECK(g_reporter->get_trade_result() == "600875.SH:22.22:0020\n"
                                                "600875.SH:22.33:0040\n");
        CHECK(g_reporter->get_market_price_result() == "600875.SH:22.22\n"
                                                       "600875.SH:22.33\n");
    }
}

TEST_CASE("Call auction stage", "[Booker]")
{
    SECTION("No trade after call auction")
    {
        trade::booker::Booker booker({}, reporter());

        booker.add(OrderCreator::ororder_tick(0, LIMIT, "600875.SH", SELL, 22.33, 20, 915000));
        booker.add(OrderCreator::ororder_tick(1, LIMIT, "600875.SH", SELL, 33.22, 40, 915000));
        booker.add(OrderCreator::ororder_tick(2, LIMIT, "600875.SH", SELL, 33.33, 80, 915000));
        booker.add(OrderCreator::ororder_tick(3, LIMIT, "600875.SH", BUY, 22.22, 100, 915000));
        booker.add(OrderCreator::ororder_tick(4, LIMIT, "600875.SH", BUY, 22.33, 100, 915000));
        booker.add(OrderCreator::ororder_tick(5, LIMIT, "600875.SH", BUY, 33.22, 100, 915000));

        booker.trade(OrderCreator::trade_tick(1, 5, "600875.SH", 33.22, 40, 925000));
        booker.trade(OrderCreator::trade_tick(0, 5, "600875.SH", 33.22, 20, 925000));

        booker.switch_to_continuous_stage();

        CHECK(g_reporter->get_trade_result() == "600875.SH:33.22:0040\n"
                                                "600875.SH:33.22:0020\n");
        CHECK(g_reporter->get_market_price_result() == "600875.SH:33.22\n"
                                                       "600875.SH:33.22\n");
    }

    SECTION("Continuous trade after call auction")
    {
        trade::booker::Booker booker({}, reporter());

        booker.add(OrderCreator::ororder_tick(0, LIMIT, "600875.SH", SELL, 22.33, 20, 915000));
        booker.add(OrderCreator::ororder_tick(1, LIMIT, "600875.SH", SELL, 33.22, 40, 915000));
        booker.add(OrderCreator::ororder_tick(2, LIMIT, "600875.SH", SELL, 33.33, 80, 915000));
        booker.add(OrderCreator::ororder_tick(3, LIMIT, "600875.SH", BUY, 22.22, 100, 915000));
        booker.add(OrderCreator::ororder_tick(4, LIMIT, "600875.SH", BUY, 22.33, 100, 915000));
        booker.add(OrderCreator::ororder_tick(5, LIMIT, "600875.SH", BUY, 33.22, 100, 915000));

        booker.trade(OrderCreator::trade_tick(1, 5, "600875.SH", 33.22, 40, 925000));
        booker.trade(OrderCreator::trade_tick(0, 5, "600875.SH", 33.22, 20, 925000));

        booker.switch_to_continuous_stage();

        booker.add(OrderCreator::ororder_tick(6, MARKET, "600875.SH", BUY, 0, 80, 930000));
        booker.add(OrderCreator::ororder_tick(7, MARKET, "600875.SH", SELL, 0, 240, 930000));

        CHECK(g_reporter->get_trade_result() == "600875.SH:33.22:0040\n"
                                                "600875.SH:33.22:0020\n"
                                                "600875.SH:33.33:0080\n"
                                                "600875.SH:33.22:0040\n"
                                                "600875.SH:22.33:0100\n"
                                                "600875.SH:22.22:0100\n");
        CHECK(g_reporter->get_market_price_result() == "600875.SH:33.22\n"
                                                       "600875.SH:33.22\n"
                                                       "600875.SH:33.33\n"
                                                       "600875.SH:33.22\n"
                                                       "600875.SH:22.33\n"
                                                       "600875.SH:22.22\n");
    }
}
