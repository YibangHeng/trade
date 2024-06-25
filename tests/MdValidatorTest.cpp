#include <catch.hpp>
#include <thread>

#include "libbooker/MdValidator.h"

TEST_CASE("Market data checking", "[MdValidator]")
{
    trade::booker::MdValidator md_validator;

    /// Mock l2 data.
    const auto l2_tick_0 = std::make_shared<trade::types::L2Tick>();
    l2_tick_0->set_symbol("600875.SH");
    l2_tick_0->set_price(22.22);
    l2_tick_0->set_quantity(100);
    l2_tick_0->set_ask_unique_id(10001);
    l2_tick_0->set_bid_unique_id(10002);

    l2_tick_0->set_sell_price_3(11.11);
    l2_tick_0->set_sell_quantity_3(1000);
    l2_tick_0->set_sell_price_2(11.11);
    l2_tick_0->set_sell_quantity_2(1000);
    l2_tick_0->set_sell_price_1(11.11);
    l2_tick_0->set_sell_quantity_1(1000);
    l2_tick_0->set_buy_price_1(11.11);
    l2_tick_0->set_buy_quantity_1(1000);
    l2_tick_0->set_buy_price_2(11.11);
    l2_tick_0->set_buy_quantity_2(1000);
    l2_tick_0->set_buy_price_3(11.11);
    l2_tick_0->set_buy_quantity_3(1000);

    const auto l2_tick_1 = std::make_shared<trade::types::L2Tick>();
    l2_tick_1->set_symbol("600875.SH");
    l2_tick_1->set_price(22.33);
    l2_tick_1->set_quantity(200);
    l2_tick_1->set_ask_unique_id(20001);
    l2_tick_1->set_bid_unique_id(20002);

    l2_tick_1->set_sell_price_3(22.22);
    l2_tick_1->set_sell_quantity_3(2000);
    l2_tick_1->set_sell_price_2(22.22);
    l2_tick_1->set_sell_quantity_2(2000);
    l2_tick_1->set_sell_price_1(22.22);
    l2_tick_1->set_sell_quantity_1(2000);
    l2_tick_1->set_buy_price_1(22.22);
    l2_tick_1->set_buy_quantity_1(2000);
    l2_tick_1->set_buy_price_2(22.22);
    l2_tick_1->set_buy_quantity_2(2000);
    l2_tick_1->set_buy_price_3(22.22);
    l2_tick_1->set_buy_quantity_3(2000);

    const auto l2_tick_2 = std::make_shared<trade::types::L2Tick>();
    l2_tick_2->set_symbol("600875.SH");
    l2_tick_2->set_price(33.22);
    l2_tick_2->set_quantity(300);
    l2_tick_2->set_ask_unique_id(30001);
    l2_tick_2->set_bid_unique_id(30002);

    l2_tick_2->set_sell_price_3(33.33);
    l2_tick_2->set_sell_quantity_3(3000);
    l2_tick_2->set_sell_price_2(22.33);
    l2_tick_2->set_sell_quantity_2(3000);
    l2_tick_2->set_sell_price_1(11.33);
    l2_tick_2->set_sell_quantity_1(3000);
    l2_tick_2->set_buy_price_1(11.33);
    l2_tick_2->set_buy_quantity_1(3000);
    l2_tick_2->set_buy_price_2(22.33);
    l2_tick_2->set_buy_quantity_2(3000);
    l2_tick_2->set_buy_price_3(33.33);
    l2_tick_2->set_buy_quantity_3(3000);

    const auto l2_tick_3 = std::make_shared<trade::types::L2Tick>();
    l2_tick_3->set_symbol("600875.SH");
    l2_tick_3->set_price(33.33);
    l2_tick_3->set_quantity(400);
    l2_tick_3->set_ask_unique_id(40001);
    l2_tick_3->set_bid_unique_id(40002);

    l2_tick_3->set_sell_price_3(44.44);
    l2_tick_3->set_sell_quantity_3(4000);
    l2_tick_3->set_sell_price_2(44.44);
    l2_tick_3->set_sell_quantity_2(4000);
    l2_tick_3->set_sell_price_1(44.44);
    l2_tick_3->set_sell_quantity_1(4000);
    l2_tick_3->set_buy_price_1(44.44);
    l2_tick_3->set_buy_quantity_1(4000);
    l2_tick_3->set_buy_price_2(44.44);
    l2_tick_3->set_buy_quantity_2(4000);
    l2_tick_3->set_buy_price_3(44.44);
    l2_tick_3->set_buy_quantity_3(4000);

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
        trade_tick_0->set_exec_price(22.22);
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
        trade_tick_2->set_exec_price(33.22);
        trade_tick_2->set_exec_quantity(300);

        const auto trade_tick_3 = std::make_shared<trade::types::TradeTick>();
        trade_tick_3->set_ask_unique_id(40001);
        trade_tick_3->set_bid_unique_id(40002);
        trade_tick_3->set_symbol("600875.SH");
        trade_tick_3->set_exec_price(22.33); /// Make price different from previous tick.
        trade_tick_3->set_exec_quantity(400);

        const auto trade_tick_4 = std::make_shared<trade::types::TradeTick>();
        trade_tick_4->set_ask_unique_id(40001);
        trade_tick_4->set_bid_unique_id(40002);
        trade_tick_4->set_symbol("600875.SH");
        trade_tick_4->set_exec_price(33.33);
        trade_tick_4->set_exec_quantity(500); /// Make quantity different from previous tick.

        SECTION("Check success")
        {
            CHECK((md_validator.check(trade_tick_0)));
            CHECK((md_validator.check(trade_tick_1)));
            CHECK((md_validator.check(trade_tick_2)));
        }

        SECTION("Check fail")
        {
            CHECK(!(md_validator.check(trade_tick_3)));
            CHECK(!(md_validator.check(trade_tick_4)));
        }
    }

    SECTION("Checking via l2 snap")
    {
        /// Reuse some previously generated mock L2 tick data.

        const auto exchange_l2_tick_3 = std::make_shared<trade::types::L2Tick>();
        exchange_l2_tick_3->set_symbol("600875.SH");
        exchange_l2_tick_3->set_price(33.33);
        exchange_l2_tick_3->set_quantity(400);
        exchange_l2_tick_3->set_ask_unique_id(40001);
        exchange_l2_tick_3->set_bid_unique_id(40002);

        exchange_l2_tick_3->set_sell_price_3(44.44);
        exchange_l2_tick_3->set_sell_quantity_3(4000);
        exchange_l2_tick_3->set_sell_price_2(44.44);
        exchange_l2_tick_3->set_sell_quantity_2(4000);
        exchange_l2_tick_3->set_sell_price_1(44.44);
        exchange_l2_tick_3->set_sell_quantity_1(5000); /// Make sell quantity 1 different from previous tick.
        exchange_l2_tick_3->set_buy_price_1(44.44);
        exchange_l2_tick_3->set_buy_quantity_1(4000);
        exchange_l2_tick_3->set_buy_price_2(44.44);
        exchange_l2_tick_3->set_buy_quantity_2(4000);
        exchange_l2_tick_3->set_buy_price_3(44.44);
        exchange_l2_tick_3->set_buy_quantity_3(4000);

        const auto exchange_l2_tick_4 = std::make_shared<trade::types::L2Tick>();
        exchange_l2_tick_4->set_symbol("600875.SH");
        exchange_l2_tick_4->set_price(33.33);
        exchange_l2_tick_4->set_quantity(400);
        exchange_l2_tick_4->set_ask_unique_id(40001);
        exchange_l2_tick_4->set_bid_unique_id(40002);

        exchange_l2_tick_4->set_sell_price_3(44.44);
        exchange_l2_tick_4->set_sell_quantity_3(4000);
        exchange_l2_tick_4->set_sell_price_2(44.44);
        exchange_l2_tick_4->set_sell_quantity_2(4000);
        exchange_l2_tick_4->set_sell_price_1(44.44);
        exchange_l2_tick_4->set_sell_quantity_1(4000);
        exchange_l2_tick_4->set_buy_price_1(44.55); /// Make buy price 1 different from previous tick.
        exchange_l2_tick_4->set_buy_quantity_1(4000);
        exchange_l2_tick_4->set_buy_price_2(44.44);
        exchange_l2_tick_4->set_buy_quantity_2(4000);
        exchange_l2_tick_4->set_buy_price_3(44.44);
        exchange_l2_tick_4->set_buy_quantity_3(4000);

        SECTION("Check success")
        {
            CHECK((md_validator.check(l2_tick_0)));
            CHECK((md_validator.check(l2_tick_1)));
            CHECK((md_validator.check(l2_tick_2)));
        }

        SECTION("Check fail")
        {
            CHECK(!(md_validator.check(exchange_l2_tick_3)));
            CHECK(!(md_validator.check(exchange_l2_tick_4)));
        }
    }
}
