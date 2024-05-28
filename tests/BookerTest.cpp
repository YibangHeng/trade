#include <catch.hpp>

#include "libbooker/Booker.h"
#include "libreporter/NopReporter.hpp"
#include "networks.pb.h"

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

std::shared_ptr<trade::types::OrderTick> create_order(
    const int64_t unique_id,
    const trade::types::OrderType order_type,
    std::string symbol,
    const trade::types::SideType side,
    const double price,
    const int64_t quantity
)
{
    auto order_tick = std::make_shared<trade::types::OrderTick>();

    order_tick->set_unique_id(unique_id);
    order_tick->set_order_type(order_type);
    order_tick->set_symbol(symbol);
    order_tick->set_side(side);
    order_tick->set_price(price);
    order_tick->set_quantity(quantity);

    return order_tick;
}

#define LIMIT trade::types::OrderType::limit
#define MARKET trade::types::OrderType::market
#define BEST_PRICE trade::types::OrderType::best_price
#define CANCEL trade::types::OrderType::cancel

#define BUY trade::types::SideType::buy
#define SELL trade::types::SideType::sell

/// TODO: Not enabled yet for .clang-format reasons.
#define T(symbol, price, quantity) #symbol ":" #price ":" #quantity "\n"
#define M(symbol, price) #symbol ":" #price "\n"

TEST_CASE("Limit order matching with 1:1 matching", "[Booker]")
{
    SECTION("Sell after buy with same price")
    {
        trade::broker::Booker booker({}, reporter());

        booker.add(create_order(1, LIMIT, "600875.SH", BUY, 22.33, 100));
        booker.add(create_order(2, LIMIT, "600875.SH", SELL, 22.33, 100));

        CHECK(g_reporter->get_trade_result() == "600875.SH:22.33:0100\n");
        CHECK(g_reporter->get_market_price_result() == "600875.SH:22.33\n");
    }

    SECTION("Buy after sell with same price")
    {
        trade::broker::Booker booker({}, reporter());

        booker.add(create_order(1, LIMIT, "600875.SH", SELL, 22.33, 100));
        booker.add(create_order(2, LIMIT, "600875.SH", BUY, 22.33, 100));

        CHECK(g_reporter->get_trade_result() == "600875.SH:22.33:0100\n");
        CHECK(g_reporter->get_market_price_result() == "600875.SH:22.33\n");
    }

    SECTION("Sell after buy with lower price")
    {
        trade::broker::Booker booker({}, reporter());

        booker.add(create_order(1, LIMIT, "600875.SH", BUY, 33.22, 100));
        booker.add(create_order(2, LIMIT, "600875.SH", SELL, 22.33, 100));

        CHECK(g_reporter->get_trade_result() == "600875.SH:33.22:0100\n");
        CHECK(g_reporter->get_market_price_result() == "600875.SH:33.22\n");
    }

    SECTION("Buy after sell with higher price")
    {
        trade::broker::Booker booker({}, reporter());

        booker.add(create_order(1, LIMIT, "600875.SH", SELL, 22.33, 100));
        booker.add(create_order(2, LIMIT, "600875.SH", BUY, 33.22, 100));

        CHECK(g_reporter->get_trade_result() == "600875.SH:22.33:0100\n");
        CHECK(g_reporter->get_market_price_result() == "600875.SH:22.33\n");
    }

    SECTION("Sell after buy with higher price")
    {
        trade::broker::Booker booker({}, reporter());

        booker.add(create_order(1, LIMIT, "600875.SH", BUY, 22.33, 100));
        booker.add(create_order(2, LIMIT, "600875.SH", SELL, 33.22, 100));

        CHECK(g_reporter->get_trade_result().empty());
        CHECK(g_reporter->get_market_price_result().empty());
    }

    SECTION("Buy after sell with lower price")
    {
        trade::broker::Booker booker({}, reporter());

        booker.add(create_order(1, LIMIT, "600875.SH", SELL, 33.22, 100));
        booker.add(create_order(2, LIMIT, "600875.SH", BUY, 22.33, 100));

        CHECK(g_reporter->get_trade_result().empty());
        CHECK(g_reporter->get_market_price_result().empty());
    }
}

TEST_CASE("Limit order matching with M:N matching", "[Booker]")
{
    SECTION("Buy after sell with same price")
    {
        trade::broker::Booker booker({}, reporter());

        booker.add(create_order(1, LIMIT, "600875.SH", SELL, 22.33, 20));
        booker.add(create_order(2, LIMIT, "600875.SH", SELL, 22.33, 40));
        booker.add(create_order(3, LIMIT, "600875.SH", SELL, 22.33, 80));
        booker.add(create_order(4, LIMIT, "600875.SH", BUY, 22.33, 100));
        booker.add(create_order(5, LIMIT, "600875.SH", BUY, 22.33, 100));

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
        trade::broker::Booker booker({}, reporter());

        booker.add(create_order(1, LIMIT, "600875.SH", SELL, 22.33, 20));
        booker.add(create_order(2, LIMIT, "600875.SH", SELL, 22.33, 40));
        booker.add(create_order(3, LIMIT, "600875.SH", SELL, 22.33, 80));
        booker.add(create_order(4, LIMIT, "600875.SH", BUY, 22.33, 100));
        booker.add(create_order(5, LIMIT, "600875.SH", BUY, 22.33, 100));

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
        trade::broker::Booker booker({}, reporter());

        booker.add(create_order(1, LIMIT, "600875.SH", BUY, 33.22, 20));
        booker.add(create_order(2, LIMIT, "600875.SH", BUY, 33.22, 40));
        booker.add(create_order(3, LIMIT, "600875.SH", BUY, 33.22, 80));
        booker.add(create_order(4, LIMIT, "600875.SH", SELL, 22.33, 100));
        booker.add(create_order(5, LIMIT, "600875.SH", SELL, 22.33, 100));

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
        trade::broker::Booker booker({}, reporter());

        booker.add(create_order(1, LIMIT, "600875.SH", SELL, 22.33, 20));
        booker.add(create_order(2, LIMIT, "600875.SH", SELL, 22.33, 40));
        booker.add(create_order(3, LIMIT, "600875.SH", SELL, 22.33, 80));
        booker.add(create_order(4, LIMIT, "600875.SH", BUY, 33.22, 100));
        booker.add(create_order(5, LIMIT, "600875.SH", BUY, 33.22, 100));

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
        trade::broker::Booker booker({}, reporter());

        booker.add(create_order(1, LIMIT, "600875.SH", BUY, 22.33, 20));
        booker.add(create_order(2, LIMIT, "600875.SH", BUY, 33.22, 40));
        booker.add(create_order(3, LIMIT, "600875.SH", BUY, 33.33, 80));
        booker.add(create_order(4, LIMIT, "600875.SH", SELL, 33.22, 100));
        booker.add(create_order(5, LIMIT, "600875.SH", SELL, 33.22, 100));

        CHECK(g_reporter->get_trade_result() == "600875.SH:33.33:0080\n"
                                                "600875.SH:33.22:0020\n"
                                                "600875.SH:33.22:0020\n");
        CHECK(g_reporter->get_market_price_result() == "600875.SH:33.33\n"
                                                       "600875.SH:33.22\n"
                                                       "600875.SH:33.22\n");
    }

    SECTION("Buy after sell with step price")
    {
        trade::broker::Booker booker({}, reporter());

        booker.add(create_order(1, LIMIT, "600875.SH", SELL, 22.22, 20));
        booker.add(create_order(2, LIMIT, "600875.SH", SELL, 22.33, 40));
        booker.add(create_order(3, LIMIT, "600875.SH", SELL, 33.22, 80));
        booker.add(create_order(4, LIMIT, "600875.SH", BUY, 22.33, 100));
        booker.add(create_order(5, LIMIT, "600875.SH", BUY, 22.33, 100));

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
        trade::broker::Booker booker({}, reporter());

        booker.add(create_order(1, LIMIT, "600875.SH", BUY, 22.33, 50));
        booker.add(create_order(1, CANCEL, "600875.SH", BUY, 22.33, 50));
        booker.add(create_order(2, LIMIT, "600875.SH", SELL, 22.33, 100));

        CHECK(g_reporter->get_trade_result().empty());
        CHECK(g_reporter->get_market_price_result().empty());
    }

    SECTION("Limit order with cancel after partial fill")
    {
        trade::broker::Booker booker({}, reporter());

        booker.add(create_order(1, LIMIT, "600875.SH", BUY, 22.33, 100));
        booker.add(create_order(2, LIMIT, "600875.SH", SELL, 22.33, 50));
        booker.add(create_order(1, CANCEL, "600875.SH", BUY, 22.33, 100));
        booker.add(create_order(3, LIMIT, "600875.SH", SELL, 22.33, 50));

        CHECK(g_reporter->get_trade_result() == "600875.SH:22.33:0050\n");
        CHECK(g_reporter->get_market_price_result() == "600875.SH:22.33\n");
    }

    SECTION("Limit order with cancel after full fill")
    {
        trade::broker::Booker booker({}, reporter());

        booker.add(create_order(1, LIMIT, "600875.SH", BUY, 22.33, 100));
        booker.add(create_order(2, LIMIT, "600875.SH", SELL, 22.33, 100));
        booker.add(create_order(1, CANCEL, "600875.SH", BUY, 22.33, 100));

        CHECK(g_reporter->get_trade_result() == "600875.SH:22.33:0100\n");
        CHECK(g_reporter->get_market_price_result() == "600875.SH:22.33\n");
    }
}

TEST_CASE("Market order matching with 1:1 matching", "[Booker]")
{
    SECTION("Sell after buy with market price")
    {
        trade::broker::Booker booker({}, reporter());

        booker.add(create_order(1, LIMIT, "600875.SH", BUY, 22.33, 100));
        booker.add(create_order(2, LIMIT, "600875.SH", BUY, 33.22, 100));
        booker.add(create_order(3, MARKET, "600875.SH", SELL, 0, 100));

        CHECK(g_reporter->get_trade_result() == "600875.SH:33.22:0100\n");
        CHECK(g_reporter->get_market_price_result() == "600875.SH:33.22\n");
    }

    SECTION("Buy after sell with market price")
    {
        trade::broker::Booker booker({}, reporter());

        booker.add(create_order(1, LIMIT, "600875.SH", SELL, 22.33, 100));
        booker.add(create_order(2, LIMIT, "600875.SH", SELL, 33.22, 100));
        booker.add(create_order(3, MARKET, "600875.SH", BUY, 0, 100));

        CHECK(g_reporter->get_trade_result() == "600875.SH:22.33:0100\n");
        CHECK(g_reporter->get_market_price_result() == "600875.SH:22.33\n");
    }
}

TEST_CASE("Market order matching with M:N matching", "[Booker]")
{
    SECTION("Buy after sell with market price")
    {
        trade::broker::Booker booker({}, reporter());

        booker.add(create_order(1, LIMIT, "600875.SH", SELL, 22.33, 20));
        booker.add(create_order(2, LIMIT, "600875.SH", SELL, 22.33, 40));
        booker.add(create_order(3, LIMIT, "600875.SH", SELL, 22.33, 80));
        booker.add(create_order(4, MARKET, "600875.SH", BUY, 0, 100));
        booker.add(create_order(5, MARKET, "600875.SH", BUY, 0, 100));

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
        trade::broker::Booker booker({}, reporter());

        booker.add(create_order(1, LIMIT, "600875.SH", SELL, 22.33, 20));
        booker.add(create_order(2, LIMIT, "600875.SH", SELL, 22.33, 40));
        booker.add(create_order(3, LIMIT, "600875.SH", SELL, 22.33, 80));
        booker.add(create_order(4, MARKET, "600875.SH", BUY, 0, 100));
        booker.add(create_order(5, MARKET, "600875.SH", BUY, 0, 100));

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
        trade::broker::Booker booker({}, reporter());

        booker.add(create_order(1, LIMIT, "600875.SH", BUY, 22.33, 20));
        booker.add(create_order(2, LIMIT, "600875.SH", BUY, 33.22, 40));
        booker.add(create_order(3, LIMIT, "600875.SH", BUY, 33.33, 80));
        booker.add(create_order(4, MARKET, "600875.SH", SELL, 0, 100));
        booker.add(create_order(5, MARKET, "600875.SH", SELL, 0, 100));

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
        trade::broker::Booker booker({}, reporter());

        booker.add(create_order(1, LIMIT, "600875.SH", SELL, 22.22, 20));
        booker.add(create_order(2, LIMIT, "600875.SH", SELL, 22.33, 40));
        booker.add(create_order(3, LIMIT, "600875.SH", SELL, 33.22, 80));
        booker.add(create_order(4, MARKET, "600875.SH", BUY, 0, 100));
        booker.add(create_order(5, MARKET, "600875.SH", BUY, 0, 100));

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
        trade::broker::Booker booker({}, reporter());

        booker.add(create_order(1, LIMIT, "600875.SH", BUY, 22.33, 20));
        booker.add(create_order(2, BEST_PRICE, "600875.SH", BUY, 0, 40));
        booker.add(create_order(3, LIMIT, "600875.SH", SELL, 22.33, 100));

        CHECK(g_reporter->get_trade_result() == "600875.SH:22.33:0020\n"
                                                "600875.SH:22.33:0040\n");
        CHECK(g_reporter->get_market_price_result() == "600875.SH:22.33\n"
                                                       "600875.SH:22.33\n");
    }

    SECTION("Buy after sell with best price")
    {
        trade::broker::Booker booker({}, reporter());

        booker.add(create_order(1, LIMIT, "600875.SH", SELL, 22.33, 20));
        booker.add(create_order(2, BEST_PRICE, "600875.SH", SELL, 0, 40));
        booker.add(create_order(3, LIMIT, "600875.SH", BUY, 22.33, 100));

        CHECK(g_reporter->get_trade_result() == "600875.SH:22.33:0020\n"
                                                "600875.SH:22.33:0040\n");
        CHECK(g_reporter->get_market_price_result() == "600875.SH:22.33\n"
                                                       "600875.SH:22.33\n");
    }
}
