#include <catch.hpp>

#include <boost/interprocess/mapped_region.hpp>
#include <boost/interprocess/shared_memory_object.hpp>

#include "libreporter/ShmReporter.h"

static constexpr boost::interprocess::offset_t GB = 1024 * 1024 * 1024;

const std::string shm_name                        = "trade_data";
constexpr boost::interprocess::offset_t shm_size  = 3 * GB;

boost::interprocess::shared_memory_object shm_reader(boost::interprocess::open_or_create, "trade_data", boost::interprocess::read_only);

/// Map areas of shared memory with same size.
auto m_trade_region        = std::make_shared<boost::interprocess::mapped_region>(shm_reader, boost::interprocess::read_only, 0 * shm_size / 3, shm_size / 3);
auto m_market_price_region = std::make_shared<boost::interprocess::mapped_region>(shm_reader, boost::interprocess::read_only, 1 * shm_size / 3, shm_size / 3);
auto m_level_price_region  = std::make_shared<boost::interprocess::mapped_region>(shm_reader, boost::interprocess::read_only, 2 * shm_size / 3, shm_size / 3);

TEST_CASE("Normal writing and reading", "[ShmReporter]")
{
    const auto reporter = std::make_shared<trade::reporter::ShmReporter>();

    /// Mock trade data.
    const auto trade_0 = std::make_shared<trade::types::MdTrade>();
    trade_0->set_symbol("600875.SH");
    trade_0->set_price(22.22);
    trade_0->set_quantity(100);
    trade_0->set_sell_3(33.33);
    trade_0->set_sell_2(22.22);
    trade_0->set_sell_1(11.11);
    trade_0->set_buy_1(11.11);
    trade_0->set_buy_2(22.22);
    trade_0->set_buy_3(33.33);

    const auto trade_1 = std::make_shared<trade::types::MdTrade>();
    trade_1->set_symbol("600875.SH");
    trade_1->set_price(22.33);
    trade_1->set_quantity(200);
    trade_1->set_sell_3(33.33);
    trade_1->set_sell_2(22.22);
    trade_1->set_sell_1(11.11);
    trade_1->set_buy_1(11.11);
    trade_1->set_buy_2(22.22);
    trade_1->set_buy_3(33.33);

    const auto trade_2 = std::make_shared<trade::types::MdTrade>();
    trade_2->set_symbol("600875.SH");
    trade_2->set_price(33.22);
    trade_2->set_quantity(300);
    trade_2->set_sell_3(33.33);
    trade_2->set_sell_2(22.22);
    trade_2->set_sell_1(11.11);
    trade_2->set_buy_1(11.11);
    trade_2->set_buy_2(22.22);
    trade_2->set_buy_3(33.33);

    const auto trade_3 = std::make_shared<trade::types::MdTrade>();
    trade_3->set_symbol("600875.SH");
    trade_3->set_price(33.33);
    trade_3->set_quantity(400);
    trade_3->set_sell_3(33.33);
    trade_3->set_sell_2(22.22);
    trade_3->set_sell_1(11.11);
    trade_3->set_buy_1(11.11);
    trade_3->set_buy_2(22.22);
    trade_3->set_buy_3(33.33);

    SECTION("Write trade and read")
    {
        reporter->md_trade_generated(trade_0);
        reporter->md_trade_generated(trade_1);
        reporter->md_trade_generated(trade_2);
        reporter->md_trade_generated(trade_3);

        auto trade_p = static_cast<trade::reporter::SMMarketData*>(m_trade_region->get_address());
        CHECK(std::string(trade_p[0].symbol) == "600875.SH");
        CHECK(trade_p[0].price == 22.22);
        CHECK(trade_p[0].quantity == 100);
        CHECK(trade_p[0].sell_3 == 33.33);
        CHECK(trade_p[0].sell_2 == 22.22);
        CHECK(trade_p[0].sell_1 == 11.11);
        CHECK(trade_p[0].buy_1 == 11.11);
        CHECK(trade_p[0].buy_2 == 22.22);
        CHECK(trade_p[0].buy_3 == 33.33);

        CHECK(std::string(trade_p[1].symbol) == "600875.SH");
        CHECK(trade_p[1].price == 22.33);
        CHECK(trade_p[1].quantity == 200);
        CHECK(trade_p[1].sell_3 == 33.33);
        CHECK(trade_p[1].sell_2 == 22.22);
        CHECK(trade_p[1].sell_1 == 11.11);
        CHECK(trade_p[1].buy_1 == 11.11);
        CHECK(trade_p[1].buy_2 == 22.22);
        CHECK(trade_p[1].buy_3 == 33.33);

        CHECK(std::string(trade_p[2].symbol) == "600875.SH");
        CHECK(trade_p[2].price == 33.22);
        CHECK(trade_p[2].quantity == 300);
        CHECK(trade_p[2].sell_3 == 33.33);
        CHECK(trade_p[2].sell_2 == 22.22);
        CHECK(trade_p[2].sell_1 == 11.11);
        CHECK(trade_p[2].buy_1 == 11.11);
        CHECK(trade_p[2].buy_2 == 22.22);
        CHECK(trade_p[2].buy_3 == 33.33);

        CHECK(std::string(trade_p[3].symbol) == "600875.SH");
        CHECK(trade_p[3].price == 33.33);
        CHECK(trade_p[3].quantity == 400);
        CHECK(trade_p[3].sell_3 == 33.33);
        CHECK(trade_p[3].sell_2 == 22.22);
        CHECK(trade_p[3].sell_1 == 11.11);
        CHECK(trade_p[3].buy_1 == 11.11);
        CHECK(trade_p[3].buy_2 == 22.22);
        CHECK(trade_p[3].buy_3 == 33.33);
    }
}
