#include <catch.hpp>
#include <thread>

#include "libreporter/ShmReporter.h"

TEST_CASE("Shm writing and reading", "[ShmReporter]")
{
    static constexpr boost::interprocess::offset_t GB = 1024 * 1024 * 1024;

    /// Shared memory/named mutex name.
    const std::string shm_name       = "trade_data_for_unit_test";
    const std::string shm_mutex_name = "trade_data_mutex_for_unit_test";

    /// Shared memory size.
    constexpr boost::interprocess::offset_t shm_size = 2 * GB;

    /// Create reporter and the reporter will create shared memory
    /// automatically.
    const auto reporter = std::make_shared<trade::reporter::ShmReporter>(shm_name, shm_mutex_name, shm_size / GB);

    /// Map areas of shared memory with same size.
    boost::interprocess::shared_memory_object shm_reader(boost::interprocess::open_only, shm_name.c_str(), boost::interprocess::read_only); /// Use open_only to make sure reporter created it.
    auto m_tick_region    = std::make_shared<boost::interprocess::mapped_region>(shm_reader, boost::interprocess::read_only, 0 * shm_size / 2, shm_size / 2);
    auto m_l2_tick_region = std::make_shared<boost::interprocess::mapped_region>(shm_reader, boost::interprocess::read_only, 1 * shm_size / 2, shm_size / 2);

    SECTION("Tick writing and reading")
    {
        /// Mock tick data.
        const auto tick_0 = std::make_shared<trade::types::ExchangeTick>();
        tick_0->set_ask_unique_id(10000);
        tick_0->set_bid_unique_id(10001);
        tick_0->set_symbol("600875.SH");
        tick_0->set_exec_price(11.11);
        tick_0->set_exec_quantity(100);

        const auto tick_1 = std::make_shared<trade::types::ExchangeTick>();
        tick_1->set_ask_unique_id(20000);
        tick_1->set_bid_unique_id(20001);
        tick_1->set_symbol("600875.SH");
        tick_1->set_exec_price(22.22);
        tick_1->set_exec_quantity(200);

        const auto tick_2 = std::make_shared<trade::types::ExchangeTick>();
        tick_2->set_ask_unique_id(30000);
        tick_2->set_bid_unique_id(30001);
        tick_2->set_symbol("600875.SH");
        tick_2->set_exec_price(33.33);
        tick_2->set_exec_quantity(300);

        const auto tick_3 = std::make_shared<trade::types::ExchangeTick>();
        tick_3->set_ask_unique_id(40000);
        tick_3->set_bid_unique_id(40001);
        tick_3->set_symbol("600875.SH");
        tick_3->set_exec_price(44.44);
        tick_3->set_exec_quantity(400);

        reporter->exchange_tick_arrived(tick_0);
        reporter->exchange_tick_arrived(tick_1);
        reporter->exchange_tick_arrived(tick_2);
        reporter->exchange_tick_arrived(tick_3);

        auto shm_mate_info = static_cast<trade::reporter::SMTickMateInfo*>(m_tick_region->get_address());

        CHECK(shm_mate_info->tick_count == 4);

        auto md_current = reinterpret_cast<trade::reporter::Tick*>(shm_mate_info + 1);
        CHECK(md_current[0].shm_union_type == trade::reporter::ShmUnionType::tick_from_exchange);
        CHECK(md_current[0].ask_unique_id == 10000);
        CHECK(md_current[0].bid_unique_id == 10001);
        CHECK(std::string(md_current[0].symbol) == "600875.SH");
        CHECK(md_current[0].exec_price == 11.11);
        CHECK(md_current[0].exec_quantity == 100);

        CHECK(md_current[1].shm_union_type == trade::reporter::ShmUnionType::tick_from_exchange);
        CHECK(md_current[1].ask_unique_id == 20000);
        CHECK(md_current[1].bid_unique_id == 20001);
        CHECK(std::string(md_current[1].symbol) == "600875.SH");
        CHECK(md_current[1].exec_price == 22.22);
        CHECK(md_current[1].exec_quantity == 200);

        CHECK(md_current[2].shm_union_type == trade::reporter::ShmUnionType::tick_from_exchange);
        CHECK(md_current[2].ask_unique_id == 30000);
        CHECK(md_current[2].bid_unique_id == 30001);
        CHECK(std::string(md_current[2].symbol) == "600875.SH");
        CHECK(md_current[2].exec_price == 33.33);
        CHECK(md_current[2].exec_quantity == 300);

        CHECK(md_current[3].shm_union_type == trade::reporter::ShmUnionType::tick_from_exchange);
        CHECK(md_current[3].ask_unique_id == 40000);
        CHECK(md_current[3].bid_unique_id == 40001);
        CHECK(std::string(md_current[3].symbol) == "600875.SH");
        CHECK(md_current[3].exec_price == 44.44);
        CHECK(md_current[3].exec_quantity == 400);
    }

    SECTION("L2 tick writing and reading")
    {
        /// Mock l2 tick data.
        const auto l2_tick_0 = std::make_shared<trade::types::L2Tick>();
        l2_tick_0->set_symbol("600875.SH");
        l2_tick_0->set_price(22.22);
        l2_tick_0->set_quantity(100);
        l2_tick_0->set_sell_price_3(33.33);
        l2_tick_0->set_sell_quantity_3(3000);
        l2_tick_0->set_sell_price_2(22.22);
        l2_tick_0->set_sell_quantity_2(2000);
        l2_tick_0->set_sell_price_1(11.11);
        l2_tick_0->set_sell_quantity_1(1000);
        l2_tick_0->set_buy_price_1(11.11);
        l2_tick_0->set_buy_quantity_1(1000);
        l2_tick_0->set_buy_price_2(22.22);
        l2_tick_0->set_buy_quantity_2(2000);
        l2_tick_0->set_buy_price_3(33.33);
        l2_tick_0->set_buy_quantity_3(3000);

        const auto l2_tick_1 = std::make_shared<trade::types::L2Tick>();
        l2_tick_1->set_symbol("600875.SH");
        l2_tick_1->set_price(22.33);
        l2_tick_1->set_quantity(200);
        l2_tick_1->set_sell_price_3(33.33);
        l2_tick_1->set_sell_quantity_3(3000);
        l2_tick_1->set_sell_price_2(22.22);
        l2_tick_1->set_sell_quantity_2(2000);
        l2_tick_1->set_sell_price_1(11.11);
        l2_tick_1->set_sell_quantity_1(1000);
        l2_tick_1->set_buy_price_1(11.11);
        l2_tick_1->set_buy_quantity_1(1000);
        l2_tick_1->set_buy_price_2(22.22);
        l2_tick_1->set_buy_quantity_2(2000);
        l2_tick_1->set_buy_price_3(33.33);
        l2_tick_1->set_buy_quantity_3(3000);

        const auto l2_tick_2 = std::make_shared<trade::types::L2Tick>();
        l2_tick_2->set_symbol("600875.SH");
        l2_tick_2->set_price(33.22);
        l2_tick_2->set_quantity(300);
        l2_tick_2->set_sell_price_3(33.33);
        l2_tick_2->set_sell_quantity_3(3000);
        l2_tick_2->set_sell_price_2(22.22);
        l2_tick_2->set_sell_quantity_2(2000);
        l2_tick_2->set_sell_price_1(11.11);
        l2_tick_2->set_sell_quantity_1(1000);
        l2_tick_2->set_buy_price_1(11.11);
        l2_tick_2->set_buy_quantity_1(1000);
        l2_tick_2->set_buy_price_2(22.22);
        l2_tick_2->set_buy_quantity_2(2000);
        l2_tick_2->set_buy_price_3(33.33);
        l2_tick_2->set_buy_quantity_3(3000);

        const auto l2_tick_3 = std::make_shared<trade::types::L2Tick>();
        l2_tick_3->set_symbol("600875.SH");
        l2_tick_3->set_price(33.33);
        l2_tick_3->set_quantity(400);
        l2_tick_3->set_sell_price_3(33.33);
        l2_tick_3->set_sell_quantity_3(3000);
        l2_tick_3->set_sell_price_2(22.22);
        l2_tick_3->set_sell_quantity_2(2000);
        l2_tick_3->set_sell_price_1(11.11);
        l2_tick_3->set_sell_quantity_1(1000);
        l2_tick_3->set_buy_price_1(11.11);
        l2_tick_3->set_buy_quantity_1(1000);
        l2_tick_3->set_buy_price_2(22.22);
        l2_tick_3->set_buy_quantity_2(2000);
        l2_tick_3->set_buy_price_3(33.33);
        l2_tick_3->set_buy_quantity_3(3000);

        reporter->l2_tick_generated(l2_tick_0);
        reporter->l2_tick_generated(l2_tick_1);
        reporter->l2_tick_generated(l2_tick_2);
        reporter->l2_tick_generated(l2_tick_3);

        auto shm_mate_info = static_cast<trade::reporter::SML2TickMateInfo*>(m_l2_tick_region->get_address());

        CHECK(shm_mate_info->l2_tick_count == 4);

        auto md_current = reinterpret_cast<trade::reporter::L2Tick*>(shm_mate_info + 1);
        CHECK(md_current[0].shm_union_type == trade::reporter::ShmUnionType::self_generated_l2_tick);
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

        CHECK(md_current[1].shm_union_type == trade::reporter::ShmUnionType::self_generated_l2_tick);
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

        CHECK(md_current[2].shm_union_type == trade::reporter::ShmUnionType::self_generated_l2_tick);
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

        CHECK(md_current[3].shm_union_type == trade::reporter::ShmUnionType::self_generated_l2_tick);
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

    /// Remove shared memory and named mutex.
    boost::interprocess::shared_memory_object::remove(shm_name.c_str());
    boost::interprocess::named_upgradable_mutex::remove(shm_mutex_name.c_str());
}
