#include <catch.hpp>
#include <thread>

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
    trade_0->set_sell_price_3(33.33);
    trade_0->set_sell_quantity_3(3000);
    trade_0->set_sell_price_2(22.22);
    trade_0->set_sell_quantity_2(2000);
    trade_0->set_sell_price_1(11.11);
    trade_0->set_sell_quantity_1(1000);
    trade_0->set_buy_price_1(11.11);
    trade_0->set_buy_quantity_1(1000);
    trade_0->set_buy_price_2(22.22);
    trade_0->set_buy_quantity_2(2000);
    trade_0->set_buy_price_3(33.33);
    trade_0->set_buy_quantity_3(3000);

    const auto trade_1 = std::make_shared<trade::types::MdTrade>();
    trade_1->set_symbol("600875.SH");
    trade_1->set_price(22.33);
    trade_1->set_quantity(200);
    trade_1->set_sell_price_3(33.33);
    trade_1->set_sell_quantity_3(3000);
    trade_1->set_sell_price_2(22.22);
    trade_1->set_sell_quantity_2(2000);
    trade_1->set_sell_price_1(11.11);
    trade_1->set_sell_quantity_1(1000);
    trade_1->set_buy_price_1(11.11);
    trade_1->set_buy_quantity_1(1000);
    trade_1->set_buy_price_2(22.22);
    trade_1->set_buy_quantity_2(2000);
    trade_1->set_buy_price_3(33.33);
    trade_1->set_buy_quantity_3(3000);

    const auto trade_2 = std::make_shared<trade::types::MdTrade>();
    trade_2->set_symbol("600875.SH");
    trade_2->set_price(33.22);
    trade_2->set_quantity(300);
    trade_2->set_sell_price_3(33.33);
    trade_2->set_sell_quantity_3(3000);
    trade_2->set_sell_price_2(22.22);
    trade_2->set_sell_quantity_2(2000);
    trade_2->set_sell_price_1(11.11);
    trade_2->set_sell_quantity_1(1000);
    trade_2->set_buy_price_1(11.11);
    trade_2->set_buy_quantity_1(1000);
    trade_2->set_buy_price_2(22.22);
    trade_2->set_buy_quantity_2(2000);
    trade_2->set_buy_price_3(33.33);
    trade_2->set_buy_quantity_3(3000);

    const auto trade_3 = std::make_shared<trade::types::MdTrade>();
    trade_3->set_symbol("600875.SH");
    trade_3->set_price(33.33);
    trade_3->set_quantity(400);
    trade_3->set_sell_price_3(33.33);
    trade_3->set_sell_quantity_3(3000);
    trade_3->set_sell_price_2(22.22);
    trade_3->set_sell_quantity_2(2000);
    trade_3->set_sell_price_1(11.11);
    trade_3->set_sell_quantity_1(1000);
    trade_3->set_buy_price_1(11.11);
    trade_3->set_buy_quantity_1(1000);
    trade_3->set_buy_price_2(22.22);
    trade_3->set_buy_quantity_2(2000);
    trade_3->set_buy_price_3(33.33);
    trade_3->set_buy_quantity_3(3000);

    SECTION("Write and read in one thread")
    {
        reporter->md_trade_generated(trade_0);
        reporter->md_trade_generated(trade_1);
        reporter->md_trade_generated(trade_2);
        reporter->md_trade_generated(trade_3);

        auto shm_mate_info = static_cast<trade::reporter::SMMateInfo*>(m_trade_region->get_address());

        CHECK(shm_mate_info->market_data_count == 4);

        auto md_current = reinterpret_cast<trade::reporter::SMMarketData*>(shm_mate_info + 1);
        CHECK(std::string(md_current[0].symbol) == "600875.SH");
        CHECK(md_current[0].price == 22.22);
        CHECK(md_current[0].quantity == 100);
        CHECK(md_current[0].sell_3.price == 33.33);
        CHECK(md_current[0].sell_3.quantity == 3000);
        CHECK(md_current[0].sell_2.price == 22.22);
        CHECK(md_current[0].sell_2.quantity == 2000);
        CHECK(md_current[0].sell_1.price == 11.11);
        CHECK(md_current[0].sell_1.quantity == 1000);
        CHECK(md_current[0].buy_1.price == 11.11);
        CHECK(md_current[0].buy_1.quantity == 1000);
        CHECK(md_current[0].buy_2.price == 22.22);
        CHECK(md_current[0].buy_2.quantity == 2000);
        CHECK(md_current[0].buy_3.price == 33.33);
        CHECK(md_current[0].buy_3.quantity == 3000);

        CHECK(std::string(md_current[1].symbol) == "600875.SH");
        CHECK(md_current[1].price == 22.33);
        CHECK(md_current[1].quantity == 200);
        CHECK(md_current[1].sell_3.price == 33.33);
        CHECK(md_current[1].sell_3.quantity == 3000);
        CHECK(md_current[1].sell_2.price == 22.22);
        CHECK(md_current[1].sell_2.quantity == 2000);
        CHECK(md_current[1].sell_1.price == 11.11);
        CHECK(md_current[1].sell_1.quantity == 1000);
        CHECK(md_current[1].buy_1.price == 11.11);
        CHECK(md_current[1].buy_1.quantity == 1000);
        CHECK(md_current[1].buy_2.price == 22.22);
        CHECK(md_current[1].buy_2.quantity == 2000);
        CHECK(md_current[1].buy_3.price == 33.33);
        CHECK(md_current[1].buy_3.quantity == 3000);

        CHECK(std::string(md_current[2].symbol) == "600875.SH");
        CHECK(md_current[2].price == 33.22);
        CHECK(md_current[2].quantity == 300);
        CHECK(md_current[2].sell_3.price == 33.33);
        CHECK(md_current[2].sell_3.quantity == 3000);
        CHECK(md_current[2].sell_2.price == 22.22);
        CHECK(md_current[2].sell_2.quantity == 2000);
        CHECK(md_current[2].sell_1.price == 11.11);
        CHECK(md_current[2].sell_1.quantity == 1000);
        CHECK(md_current[2].buy_1.price == 11.11);
        CHECK(md_current[2].buy_1.quantity == 1000);
        CHECK(md_current[2].buy_2.price == 22.22);
        CHECK(md_current[2].buy_2.quantity == 2000);
        CHECK(md_current[2].buy_3.price == 33.33);
        CHECK(md_current[2].buy_3.quantity == 3000);

        CHECK(std::string(md_current[3].symbol) == "600875.SH");
        CHECK(md_current[3].price == 33.33);
        CHECK(md_current[3].quantity == 400);
        CHECK(md_current[3].sell_3.price == 33.33);
        CHECK(md_current[3].sell_3.quantity == 3000);
        CHECK(md_current[3].sell_2.price == 22.22);
        CHECK(md_current[3].sell_2.quantity == 2000);
        CHECK(md_current[3].sell_1.price == 11.11);
        CHECK(md_current[3].sell_1.quantity == 1000);
        CHECK(md_current[3].buy_1.price == 11.11);
        CHECK(md_current[3].buy_1.quantity == 1000);
        CHECK(md_current[3].buy_2.price == 22.22);
        CHECK(md_current[3].buy_2.quantity == 2000);
        CHECK(md_current[3].buy_3.price == 33.33);
        CHECK(md_current[3].buy_3.quantity == 3000);
    }

    SECTION("Write and read in one thread")
    {
        reporter->md_trade_generated(trade_0);
        reporter->md_trade_generated(trade_1);
        reporter->md_trade_generated(trade_2);
        reporter->md_trade_generated(trade_3);

        std::thread reader([] {
            boost::interprocess::named_upgradable_mutex read_mutex(boost::interprocess::open_or_create, "trade_data_mutex");
            boost::interprocess::scoped_lock lock(read_mutex);

            auto shm_mate_info = static_cast<trade::reporter::SMMateInfo*>(m_trade_region->get_address());

            CHECK(shm_mate_info->market_data_count == 4);

            auto md_current = reinterpret_cast<trade::reporter::SMMarketData*>(shm_mate_info + 1);
            CHECK(std::string(md_current[0].symbol) == "600875.SH");
            CHECK(md_current[0].price == 22.22);
            CHECK(md_current[0].quantity == 100);
            CHECK(md_current[0].sell_3.price == 33.33);
            CHECK(md_current[0].sell_3.quantity == 3000);
            CHECK(md_current[0].sell_2.price == 22.22);
            CHECK(md_current[0].sell_2.quantity == 2000);
            CHECK(md_current[0].sell_1.price == 11.11);
            CHECK(md_current[0].sell_1.quantity == 1000);
            CHECK(md_current[0].buy_1.price == 11.11);
            CHECK(md_current[0].buy_1.quantity == 1000);
            CHECK(md_current[0].buy_2.price == 22.22);
            CHECK(md_current[0].buy_2.quantity == 2000);
            CHECK(md_current[0].buy_3.price == 33.33);
            CHECK(md_current[0].buy_3.quantity == 3000);

            CHECK(std::string(md_current[1].symbol) == "600875.SH");
            CHECK(md_current[1].price == 22.33);
            CHECK(md_current[1].quantity == 200);
            CHECK(md_current[1].sell_3.price == 33.33);
            CHECK(md_current[1].sell_3.quantity == 3000);
            CHECK(md_current[1].sell_2.price == 22.22);
            CHECK(md_current[1].sell_2.quantity == 2000);
            CHECK(md_current[1].sell_1.price == 11.11);
            CHECK(md_current[1].sell_1.quantity == 1000);
            CHECK(md_current[1].buy_1.price == 11.11);
            CHECK(md_current[1].buy_1.quantity == 1000);
            CHECK(md_current[1].buy_2.price == 22.22);
            CHECK(md_current[1].buy_2.quantity == 2000);
            CHECK(md_current[1].buy_3.price == 33.33);
            CHECK(md_current[1].buy_3.quantity == 3000);

            CHECK(std::string(md_current[2].symbol) == "600875.SH");
            CHECK(md_current[2].price == 33.22);
            CHECK(md_current[2].quantity == 300);
            CHECK(md_current[2].sell_3.price == 33.33);
            CHECK(md_current[2].sell_3.quantity == 3000);
            CHECK(md_current[2].sell_2.price == 22.22);
            CHECK(md_current[2].sell_2.quantity == 2000);
            CHECK(md_current[2].sell_1.price == 11.11);
            CHECK(md_current[2].sell_1.quantity == 1000);
            CHECK(md_current[2].buy_1.price == 11.11);
            CHECK(md_current[2].buy_1.quantity == 1000);
            CHECK(md_current[2].buy_2.price == 22.22);
            CHECK(md_current[2].buy_2.quantity == 2000);
            CHECK(md_current[2].buy_3.price == 33.33);
            CHECK(md_current[2].buy_3.quantity == 3000);

            CHECK(std::string(md_current[3].symbol) == "600875.SH");
            CHECK(md_current[3].price == 33.33);
            CHECK(md_current[3].quantity == 400);
            CHECK(md_current[3].sell_3.price == 33.33);
            CHECK(md_current[3].sell_3.quantity == 3000);
            CHECK(md_current[3].sell_2.price == 22.22);
            CHECK(md_current[3].sell_2.quantity == 2000);
            CHECK(md_current[3].sell_1.price == 11.11);
            CHECK(md_current[3].sell_1.quantity == 1000);
            CHECK(md_current[3].buy_1.price == 11.11);
            CHECK(md_current[3].buy_1.quantity == 1000);
            CHECK(md_current[3].buy_2.price == 22.22);
            CHECK(md_current[3].buy_2.quantity == 2000);
            CHECK(md_current[3].buy_3.price == 33.33);
            CHECK(md_current[3].buy_3.quantity == 3000);
        });

        reader.join();
    }
}
