#include <catch.hpp>
#include <thread>

#include "libbooker/MdValidator.h"

TEST_CASE("L2 snap checking", "[MdValidator]")
{
    trade::booker::MdValidator md_validator;

    /// Mock trade data.
    const auto trade_0 = std::make_shared<trade::types::MdTrade>();
    trade_0->set_symbol("600875.SH");
    trade_0->set_price(22.22);
    trade_0->set_quantity(100);
    trade_0->set_sell_price_3(11.11);
    trade_0->set_sell_quantity_3(1000);
    trade_0->set_sell_price_2(11.11);
    trade_0->set_sell_quantity_2(1000);
    trade_0->set_sell_price_1(11.11);
    trade_0->set_sell_quantity_1(1000);
    trade_0->set_buy_price_1(11.11);
    trade_0->set_buy_quantity_1(1000);
    trade_0->set_buy_price_2(11.11);
    trade_0->set_buy_quantity_2(1000);
    trade_0->set_buy_price_3(11.11);
    trade_0->set_buy_quantity_3(1000);

    const auto trade_1 = std::make_shared<trade::types::MdTrade>();
    trade_1->set_symbol("600875.SH");
    trade_1->set_price(22.33);
    trade_1->set_quantity(200);
    trade_1->set_sell_price_3(22.22);
    trade_1->set_sell_quantity_3(2000);
    trade_1->set_sell_price_2(22.22);
    trade_1->set_sell_quantity_2(2000);
    trade_1->set_sell_price_1(22.22);
    trade_1->set_sell_quantity_1(2000);
    trade_1->set_buy_price_1(22.22);
    trade_1->set_buy_quantity_1(2000);
    trade_1->set_buy_price_2(22.22);
    trade_1->set_buy_quantity_2(2000);
    trade_1->set_buy_price_3(22.22);
    trade_1->set_buy_quantity_3(2000);

    const auto trade_2 = std::make_shared<trade::types::MdTrade>();
    trade_2->set_symbol("600875.SH");
    trade_2->set_price(33.22);
    trade_2->set_quantity(300);
    trade_2->set_sell_price_3(33.33);
    trade_2->set_sell_quantity_3(3000);
    trade_2->set_sell_price_2(22.33);
    trade_2->set_sell_quantity_2(3000);
    trade_2->set_sell_price_1(11.33);
    trade_2->set_sell_quantity_1(3000);
    trade_2->set_buy_price_1(11.33);
    trade_2->set_buy_quantity_1(3000);
    trade_2->set_buy_price_2(22.33);
    trade_2->set_buy_quantity_2(3000);
    trade_2->set_buy_price_3(33.33);
    trade_2->set_buy_quantity_3(3000);

    const auto trade_3 = std::make_shared<trade::types::MdTrade>();
    trade_3->set_symbol("600875.SH");
    trade_3->set_price(33.33);
    trade_3->set_quantity(400);
    trade_3->set_sell_price_3(44.44);
    trade_3->set_sell_quantity_3(4000);
    trade_3->set_sell_price_2(44.44);
    trade_3->set_sell_quantity_2(4000);
    trade_3->set_sell_price_1(44.44);
    trade_3->set_sell_quantity_1(4000);
    trade_3->set_buy_price_1(44.44);
    trade_3->set_buy_quantity_1(4000);
    trade_3->set_buy_price_2(44.44);
    trade_3->set_buy_quantity_2(4000);
    trade_3->set_buy_price_3(44.44);
    trade_3->set_buy_quantity_3(4000);

    md_validator.md_trade_generated(trade_0);
    md_validator.md_trade_generated(trade_1);
    md_validator.md_trade_generated(trade_2);

    SECTION("Check success")
    {
        CHECK((md_validator.check(trade_0)));
        CHECK((md_validator.check(trade_1)));
        CHECK((md_validator.check(trade_2)));
    }

    SECTION("Check fail")
    {
        CHECK(!(md_validator.check(trade_3)));
    }
}
