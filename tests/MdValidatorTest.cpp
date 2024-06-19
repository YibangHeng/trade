#include <catch.hpp>
#include <thread>

#include "libbooker/MdValidator.h"

TEST_CASE("L2 snap checking", "[MdValidator]")
{
    trade::booker::MdValidator md_validator;

    /// Mock trade data.
    const auto l2_tick_0 = std::make_shared<trade::types::L2Tick>();
    l2_tick_0->set_symbol("600875.SH");
    l2_tick_0->set_price(22.22);
    l2_tick_0->set_quantity(100);
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

    SECTION("Check success")
    {
        CHECK((md_validator.check(l2_tick_0)));
        CHECK((md_validator.check(l2_tick_1)));
        CHECK((md_validator.check(l2_tick_2)));
    }

    SECTION("Check fail")
    {
        CHECK(!(md_validator.check(l2_tick_3)));
    }
}
