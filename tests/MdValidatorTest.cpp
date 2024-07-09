#include <catch.hpp>
#include <thread>

#include "libbooker/MdValidator.h"

TEST_CASE("Market data checking", "[MdValidator]")
{
    trade::booker::MdValidator md_validator;

    /// Mock l2 data.
    const auto l2_tick_0 = std::make_shared<trade::types::L2Tick>();
    l2_tick_0->set_symbol("600875.SH");
    l2_tick_0->set_price(22.33);
    l2_tick_0->set_quantity(100);
    l2_tick_0->set_ask_unique_id(10001);
    l2_tick_0->set_bid_unique_id(10002);

    const auto l2_tick_1 = std::make_shared<trade::types::L2Tick>();
    l2_tick_1->set_symbol("600875.SH");
    l2_tick_1->set_price(22.33);
    l2_tick_1->set_quantity(200);
    l2_tick_1->set_ask_unique_id(20001);
    l2_tick_1->set_bid_unique_id(20002);

    const auto l2_tick_2 = std::make_shared<trade::types::L2Tick>();
    l2_tick_2->set_symbol("600875.SH");
    l2_tick_2->set_price(22.33);
    l2_tick_2->set_quantity(300);
    l2_tick_2->set_ask_unique_id(30001);
    l2_tick_2->set_bid_unique_id(30002);

    const auto l2_tick_3 = std::make_shared<trade::types::L2Tick>();
    l2_tick_3->set_symbol("600875.SH");
    l2_tick_3->set_price(22.33);
    l2_tick_3->set_quantity(400);
    l2_tick_3->set_ask_unique_id(40001);
    l2_tick_3->set_bid_unique_id(40002);

    md_validator.l2_tick_generated(l2_tick_0);
    md_validator.l2_tick_generated(l2_tick_1);
    md_validator.l2_tick_generated(l2_tick_2);
    md_validator.l2_tick_generated(l2_tick_3);

    SECTION("Checking via trade tick")
    {
        /// Mock trade data.
        const auto trade_tick_0 = std::make_shared<trade::types::TradeTick>();
        trade_tick_0->set_ask_unique_id(10001);
        trade_tick_0->set_bid_unique_id(10002);
        trade_tick_0->set_symbol("600875.SH");
        trade_tick_0->set_exec_price(22.33);
        trade_tick_0->set_exec_quantity(100);

        const auto trade_tick_1 = std::make_shared<trade::types::TradeTick>();
        trade_tick_1->set_ask_unique_id(20001);
        trade_tick_1->set_bid_unique_id(20002);
        trade_tick_1->set_symbol("600875.SH");
        trade_tick_1->set_exec_price(22.33);
        trade_tick_1->set_exec_quantity(200);

        const auto trade_tick_2 = std::make_shared<trade::types::TradeTick>();
        trade_tick_2->set_ask_unique_id(30001);
        trade_tick_2->set_bid_unique_id(30002);
        trade_tick_2->set_symbol("600875.SH");
        trade_tick_2->set_exec_price(22.33);
        trade_tick_2->set_exec_quantity(300);

        const auto trade_tick_3 = std::make_shared<trade::types::TradeTick>();
        trade_tick_3->set_ask_unique_id(50001); /// Make ask unique id different from previous tick.
        trade_tick_3->set_bid_unique_id(10002);
        trade_tick_3->set_symbol("600875.SH");
        trade_tick_3->set_exec_price(22.33);
        trade_tick_3->set_exec_quantity(400);

        const auto trade_tick_4 = std::make_shared<trade::types::TradeTick>();
        trade_tick_4->set_ask_unique_id(10001);
        trade_tick_4->set_bid_unique_id(50002); /// Make bid unique id different from previous tick.
        trade_tick_4->set_symbol("600875.SH");
        trade_tick_4->set_exec_price(22.33);
        trade_tick_4->set_exec_quantity(500);

        SECTION("Check success")
        {
            CHECK(md_validator.check(trade_tick_0));
            CHECK(md_validator.check(trade_tick_1));
            CHECK(md_validator.check(trade_tick_2));
        }

        SECTION("Check fail")
        {
            CHECK_FALSE(md_validator.check(trade_tick_3));
            CHECK_FALSE(md_validator.check(trade_tick_4));
        }
    }
}
