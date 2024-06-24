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
    }

public:
    [[nodiscard]] std::string get_trade_result() const
    {
        return m_trade_result.str();
    }

public:
    void l2_tick_generated(const std::shared_ptr<trade::types::L2Tick> l2_tick) override
    {
        /// TODO: Add l2 quantity check.
        m_trade_result << fmt::format(
            "{}:{:05.2f}:{:04} S{:05.2f}:S{:05.2f}:S{:05.2f} B{:05.2f}:B{:05.2f}:B{:05.2f}\n",
            l2_tick->symbol(), l2_tick->price(), l2_tick->quantity(),                  /// Symbol, price and quantity of last trade.
            l2_tick->sell_price_3(), l2_tick->sell_price_2(), l2_tick->sell_price_1(), /// Sell level prices.
            l2_tick->buy_price_1(), l2_tick->buy_price_2(), l2_tick->buy_price_3()     /// Buy level prices.
        );
    }

private:
    std::ostringstream m_trade_result;
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

            CHECK(g_reporter->get_trade_result() == "600875.SH:22.33:0100 S00.00:S00.00:S00.00 B00.00:B00.00:B00.00\n");
        }

        SECTION("Buy after sell with same price")
        {
            trade::booker::Booker booker({}, reporter());

            booker.add(OrderCreator::ororder_tick(0, LIMIT, "600875.SH", SELL, 22.33, 100));
            booker.add(OrderCreator::ororder_tick(1, LIMIT, "600875.SH", BUY, 22.33, 100));

            CHECK(g_reporter->get_trade_result() == "600875.SH:22.33:0100 S00.00:S00.00:S00.00 B00.00:B00.00:B00.00\n");
        }

        SECTION("Sell after buy with lower price")
        {
            trade::booker::Booker booker({}, reporter());

            booker.add(OrderCreator::ororder_tick(0, LIMIT, "600875.SH", BUY, 33.22, 100));
            booker.add(OrderCreator::ororder_tick(1, LIMIT, "600875.SH", SELL, 22.33, 100));

            CHECK(g_reporter->get_trade_result() == "600875.SH:33.22:0100 S00.00:S00.00:S00.00 B00.00:B00.00:B00.00\n");
        }

        SECTION("Buy after sell with higher price")
        {
            trade::booker::Booker booker({}, reporter());

            booker.add(OrderCreator::ororder_tick(0, LIMIT, "600875.SH", SELL, 22.33, 100));
            booker.add(OrderCreator::ororder_tick(1, LIMIT, "600875.SH", BUY, 33.22, 100));

            CHECK(g_reporter->get_trade_result() == "600875.SH:22.33:0100 S00.00:S00.00:S00.00 B00.00:B00.00:B00.00\n");
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

            CHECK(g_reporter->get_trade_result() == "600875.SH:22.33:0020 S00.00:S00.00:S22.33 B00.00:B00.00:B00.00\n"
                                                    "600875.SH:22.33:0040 S00.00:S00.00:S22.33 B00.00:B00.00:B00.00\n"
                                                    "600875.SH:22.33:0040 S00.00:S00.00:S22.33 B00.00:B00.00:B00.00\n"
                                                    "600875.SH:22.33:0040 S00.00:S00.00:S00.00 B22.33:B00.00:B00.00\n");
        }

        SECTION("Buy after sell with same price")
        {
            trade::booker::Booker booker({}, reporter());

            booker.add(OrderCreator::ororder_tick(0, LIMIT, "600875.SH", SELL, 22.33, 20));
            booker.add(OrderCreator::ororder_tick(1, LIMIT, "600875.SH", SELL, 22.33, 40));
            booker.add(OrderCreator::ororder_tick(2, LIMIT, "600875.SH", SELL, 22.33, 80));
            booker.add(OrderCreator::ororder_tick(3, LIMIT, "600875.SH", BUY, 22.33, 100));
            booker.add(OrderCreator::ororder_tick(4, LIMIT, "600875.SH", BUY, 22.33, 100));

            CHECK(g_reporter->get_trade_result() == "600875.SH:22.33:0020 S00.00:S00.00:S22.33 B00.00:B00.00:B00.00\n"
                                                    "600875.SH:22.33:0040 S00.00:S00.00:S22.33 B00.00:B00.00:B00.00\n"
                                                    "600875.SH:22.33:0040 S00.00:S00.00:S22.33 B00.00:B00.00:B00.00\n"
                                                    "600875.SH:22.33:0040 S00.00:S00.00:S00.00 B22.33:B00.00:B00.00\n");
        }

        SECTION("Sell after buy with lower price")
        {
            trade::booker::Booker booker({}, reporter());

            booker.add(OrderCreator::ororder_tick(0, LIMIT, "600875.SH", BUY, 33.22, 20));
            booker.add(OrderCreator::ororder_tick(1, LIMIT, "600875.SH", BUY, 33.22, 40));
            booker.add(OrderCreator::ororder_tick(2, LIMIT, "600875.SH", BUY, 33.22, 80));
            booker.add(OrderCreator::ororder_tick(3, LIMIT, "600875.SH", SELL, 22.33, 100));
            booker.add(OrderCreator::ororder_tick(4, LIMIT, "600875.SH", SELL, 22.33, 100));

            CHECK(g_reporter->get_trade_result() == "600875.SH:33.22:0020 S00.00:S00.00:S00.00 B33.22:B00.00:B00.00\n"
                                                    "600875.SH:33.22:0040 S00.00:S00.00:S00.00 B33.22:B00.00:B00.00\n"
                                                    "600875.SH:33.22:0040 S00.00:S00.00:S00.00 B33.22:B00.00:B00.00\n"
                                                    "600875.SH:33.22:0040 S00.00:S00.00:S22.33 B00.00:B00.00:B00.00\n"

            );
        }

        SECTION("Buy after sell with higher price")
        {
            trade::booker::Booker booker({}, reporter());

            booker.add(OrderCreator::ororder_tick(0, LIMIT, "600875.SH", SELL, 22.33, 20));
            booker.add(OrderCreator::ororder_tick(1, LIMIT, "600875.SH", SELL, 22.33, 40));
            booker.add(OrderCreator::ororder_tick(2, LIMIT, "600875.SH", SELL, 22.33, 80));
            booker.add(OrderCreator::ororder_tick(3, LIMIT, "600875.SH", BUY, 33.22, 100));
            booker.add(OrderCreator::ororder_tick(4, LIMIT, "600875.SH", BUY, 33.22, 100));

            CHECK(g_reporter->get_trade_result() == "600875.SH:22.33:0020 S00.00:S00.00:S22.33 B00.00:B00.00:B00.00\n"
                                                    "600875.SH:22.33:0040 S00.00:S00.00:S22.33 B00.00:B00.00:B00.00\n"
                                                    "600875.SH:22.33:0040 S00.00:S00.00:S22.33 B00.00:B00.00:B00.00\n"
                                                    "600875.SH:22.33:0040 S00.00:S00.00:S00.00 B33.22:B00.00:B00.00\n");
        }

        SECTION("Sell after buy with step price")
        {
            trade::booker::Booker booker({}, reporter());

            booker.add(OrderCreator::ororder_tick(0, LIMIT, "600875.SH", BUY, 22.33, 20));
            booker.add(OrderCreator::ororder_tick(1, LIMIT, "600875.SH", BUY, 33.22, 40));
            booker.add(OrderCreator::ororder_tick(2, LIMIT, "600875.SH", BUY, 33.33, 80));
            booker.add(OrderCreator::ororder_tick(3, LIMIT, "600875.SH", SELL, 33.22, 100));
            booker.add(OrderCreator::ororder_tick(4, LIMIT, "600875.SH", SELL, 33.22, 100));

            CHECK(g_reporter->get_trade_result() == "600875.SH:33.33:0080 S00.00:S00.00:S00.00 B33.22:B22.33:B00.00\n"
                                                    "600875.SH:33.22:0020 S00.00:S00.00:S00.00 B33.22:B22.33:B00.00\n"
                                                    "600875.SH:33.22:0020 S00.00:S00.00:S33.22 B22.33:B00.00:B00.00\n");
        }

        SECTION("Buy after sell with step price")
        {
            trade::booker::Booker booker({}, reporter());

            booker.add(OrderCreator::ororder_tick(0, LIMIT, "600875.SH", SELL, 22.22, 20));
            booker.add(OrderCreator::ororder_tick(1, LIMIT, "600875.SH", SELL, 22.33, 40));
            booker.add(OrderCreator::ororder_tick(2, LIMIT, "600875.SH", SELL, 33.22, 80));
            booker.add(OrderCreator::ororder_tick(3, LIMIT, "600875.SH", BUY, 22.33, 100));
            booker.add(OrderCreator::ororder_tick(4, LIMIT, "600875.SH", BUY, 22.33, 100));

            CHECK(g_reporter->get_trade_result() == "600875.SH:22.22:0020 S00.00:S00.00:S33.22 B22.33:B00.00:B00.00\n"
                                                    "600875.SH:22.33:0040 S00.00:S00.00:S33.22 B22.33:B00.00:B00.00\n");
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

            CHECK(g_reporter->get_trade_result() == "600875.SH:22.33:0050 S00.00:S00.00:S00.00 B22.33:B00.00:B00.00\n");
        }

        SECTION("Limit order with cancel after full fill")
        {
            trade::booker::Booker booker({}, reporter());

            booker.add(OrderCreator::ororder_tick(0, LIMIT, "600875.SH", BUY, 22.33, 100));
            booker.add(OrderCreator::ororder_tick(1, LIMIT, "600875.SH", SELL, 22.33, 100));
            booker.add(OrderCreator::ororder_tick(0, CANCEL, "600875.SH", INV_SIDE, 0, 0));

            CHECK(g_reporter->get_trade_result() == "600875.SH:22.33:0100 S00.00:S00.00:S00.00 B00.00:B00.00:B00.00\n");
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

            CHECK(g_reporter->get_trade_result() == "600875.SH:33.22:0100 S00.00:S00.00:S00.00 B22.33:B00.00:B00.00\n");
        }

        SECTION("Buy after sell with market price")
        {
            trade::booker::Booker booker({}, reporter());

            booker.add(OrderCreator::ororder_tick(0, LIMIT, "600875.SH", SELL, 22.33, 100));
            booker.add(OrderCreator::ororder_tick(1, LIMIT, "600875.SH", SELL, 33.22, 100));
            booker.add(OrderCreator::ororder_tick(2, MARKET, "600875.SH", BUY, 0, 100));

            CHECK(g_reporter->get_trade_result() == "600875.SH:22.33:0100 S00.00:S00.00:S33.22 B00.00:B00.00:B00.00\n");
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

            CHECK(g_reporter->get_trade_result() == "600875.SH:22.33:0020 S00.00:S00.00:S22.33 B00.00:B00.00:B00.00\n"
                                                    "600875.SH:22.33:0040 S00.00:S00.00:S22.33 B00.00:B00.00:B00.00\n"
                                                    "600875.SH:22.33:0040 S00.00:S00.00:S22.33 B00.00:B00.00:B00.00\n"
                                                    "600875.SH:22.33:0040 S00.00:S00.00:S00.00 B00.00:B00.00:B00.00\n");
        }

        SECTION("Buy after sell with market price")
        {
            trade::booker::Booker booker({}, reporter());

            booker.add(OrderCreator::ororder_tick(0, LIMIT, "600875.SH", SELL, 22.33, 20));
            booker.add(OrderCreator::ororder_tick(1, LIMIT, "600875.SH", SELL, 22.33, 40));
            booker.add(OrderCreator::ororder_tick(2, LIMIT, "600875.SH", SELL, 22.33, 80));
            booker.add(OrderCreator::ororder_tick(3, MARKET, "600875.SH", BUY, 0, 100));
            booker.add(OrderCreator::ororder_tick(4, MARKET, "600875.SH", BUY, 0, 100));

            CHECK(g_reporter->get_trade_result() == "600875.SH:22.33:0020 S00.00:S00.00:S22.33 B00.00:B00.00:B00.00\n"
                                                    "600875.SH:22.33:0040 S00.00:S00.00:S22.33 B00.00:B00.00:B00.00\n"
                                                    "600875.SH:22.33:0040 S00.00:S00.00:S22.33 B00.00:B00.00:B00.00\n"
                                                    "600875.SH:22.33:0040 S00.00:S00.00:S00.00 B00.00:B00.00:B00.00\n");
        }

        SECTION("Sell after buy with step market price")
        {
            trade::booker::Booker booker({}, reporter());

            booker.add(OrderCreator::ororder_tick(0, LIMIT, "600875.SH", BUY, 22.33, 20));
            booker.add(OrderCreator::ororder_tick(1, LIMIT, "600875.SH", BUY, 33.22, 40));
            booker.add(OrderCreator::ororder_tick(2, LIMIT, "600875.SH", BUY, 33.33, 80));
            booker.add(OrderCreator::ororder_tick(3, MARKET, "600875.SH", SELL, 0, 100));
            booker.add(OrderCreator::ororder_tick(4, MARKET, "600875.SH", SELL, 0, 100));

            CHECK(g_reporter->get_trade_result() == "600875.SH:33.33:0080 S00.00:S00.00:S00.00 B33.22:B22.33:B00.00\n"
                                                    "600875.SH:33.22:0020 S00.00:S00.00:S00.00 B33.22:B22.33:B00.00\n"
                                                    "600875.SH:33.22:0020 S00.00:S00.00:S00.00 B00.00:B00.00:B00.00\n"
                                                    "600875.SH:22.33:0020 S00.00:S00.00:S00.00 B00.00:B00.00:B00.00\n");
        }

        SECTION("Buy after sell with step market price")
        {
            trade::booker::Booker booker({}, reporter());

            booker.add(OrderCreator::ororder_tick(0, LIMIT, "600875.SH", SELL, 22.22, 20));
            booker.add(OrderCreator::ororder_tick(1, LIMIT, "600875.SH", SELL, 22.33, 40));
            booker.add(OrderCreator::ororder_tick(2, LIMIT, "600875.SH", SELL, 33.22, 80));
            booker.add(OrderCreator::ororder_tick(3, MARKET, "600875.SH", BUY, 0, 100));
            booker.add(OrderCreator::ororder_tick(4, MARKET, "600875.SH", BUY, 0, 100));

            CHECK(g_reporter->get_trade_result() == "600875.SH:22.22:0020 S00.00:S00.00:S33.22 B00.00:B00.00:B00.00\n"
                                                    "600875.SH:22.33:0040 S00.00:S00.00:S33.22 B00.00:B00.00:B00.00\n"
                                                    "600875.SH:33.22:0040 S00.00:S00.00:S33.22 B00.00:B00.00:B00.00\n"
                                                    "600875.SH:33.22:0040 S00.00:S00.00:S00.00 B00.00:B00.00:B00.00\n");
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

            CHECK(g_reporter->get_trade_result() == "600875.SH:22.33:0020 S00.00:S00.00:S22.33 B00.00:B00.00:B00.00\n"
                                                    "600875.SH:22.33:0040 S00.00:S00.00:S22.33 B00.00:B00.00:B00.00\n");
        }

        SECTION("Buy after sell with best price")
        {
            trade::booker::Booker booker({}, reporter());

            booker.add(OrderCreator::ororder_tick(0, LIMIT, "600875.SH", SELL, 22.33, 20));
            booker.add(OrderCreator::ororder_tick(1, BEST_PRICE, "600875.SH", SELL, 0, 40));
            booker.add(OrderCreator::ororder_tick(2, LIMIT, "600875.SH", BUY, 22.33, 100));

            CHECK(g_reporter->get_trade_result() == "600875.SH:22.33:0020 S00.00:S00.00:S00.00 B22.33:B00.00:B00.00\n"
                                                    "600875.SH:22.33:0040 S00.00:S00.00:S00.00 B22.33:B00.00:B00.00\n");
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

            CHECK(g_reporter->get_trade_result() == "600875.SH:33.22:0040 S00.00:S00.00:S00.00 B00.00:B00.00:B00.00\n"
                                                    "600875.SH:33.22:0020 S00.00:S00.00:S00.00 B00.00:B00.00:B00.00\n");
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

            CHECK(g_reporter->get_trade_result() == "600875.SH:33.22:0040 S00.00:S00.00:S00.00 B00.00:B00.00:B00.00\n"
                                                    "600875.SH:33.22:0020 S00.00:S00.00:S00.00 B00.00:B00.00:B00.00\n"
                                                    "600875.SH:33.33:0080 S00.00:S00.00:S00.00 B33.22:B22.33:B22.22\n"
                                                    "600875.SH:33.22:0040 S00.00:S00.00:S00.00 B00.00:B00.00:B00.00\n"
                                                    "600875.SH:22.33:0100 S00.00:S00.00:S00.00 B00.00:B00.00:B00.00\n"
                                                    "600875.SH:22.22:0100 S00.00:S00.00:S00.00 B00.00:B00.00:B00.00\n");
        }
    }
}
