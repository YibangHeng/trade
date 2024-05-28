#include <catch.hpp>

#include "libbooker/OrderWrapper.h"
#include "libbooker/Rearranger.h"

std::shared_ptr<trade::broker::OrderWrapper> create_order(const int64_t unique_id)
{
    const auto order_tick = std::make_shared<trade::types::OrderTick>();

    order_tick->set_unique_id(unique_id);

    return std::make_shared<trade::broker::OrderWrapper>(order_tick);
}

TEST_CASE("In order pushing", "[Rearranger]")
{
    SECTION("Single pushing and popping")
    {
        trade::broker::Rearranger rearranger;

        rearranger.push(create_order(0));

        CHECK(rearranger.pop().value()->unique_id() == 0);
        CHECK(rearranger.pop().has_value() == false);
    }

    SECTION("Multiple pushing and popping")
    {
        trade::broker::Rearranger rearranger;

        rearranger.push(create_order(0));
        rearranger.push(create_order(1));
        rearranger.push(create_order(2));

        CHECK(rearranger.pop().value()->unique_id() == 0);
        CHECK(rearranger.pop().value()->unique_id() == 1);
        CHECK(rearranger.pop().value()->unique_id() == 2);
        CHECK(rearranger.pop().has_value() == false);
    }

    SECTION("Multiple pushing and multiple popping")
    {
        trade::broker::Rearranger rearranger;

        rearranger.push(create_order(0));
        CHECK(rearranger.pop().value()->unique_id() == 0);
        CHECK(rearranger.pop().has_value() == false);

        rearranger.push(create_order(1));
        rearranger.push(create_order(2));
        CHECK(rearranger.pop().value()->unique_id() == 1);
        CHECK(rearranger.pop().value()->unique_id() == 2);
        CHECK(rearranger.pop().has_value() == false);

        rearranger.push(create_order(3));
        rearranger.push(create_order(4));
        rearranger.push(create_order(5));
        CHECK(rearranger.pop().value()->unique_id() == 3);
        CHECK(rearranger.pop().value()->unique_id() == 4);
        CHECK(rearranger.pop().value()->unique_id() == 5);
        CHECK(rearranger.pop().has_value() == false);
    }
}

TEST_CASE("Out of order pushing and popping", "[Rearranger]")
{
    SECTION("Multiple pushing and popping")
    {
        trade::broker::Rearranger rearranger;

        rearranger.push(create_order(1));
        rearranger.push(create_order(0));
        rearranger.push(create_order(2));

        CHECK(rearranger.pop().value()->unique_id() == 0);
        CHECK(rearranger.pop().value()->unique_id() == 1);
        CHECK(rearranger.pop().value()->unique_id() == 2);
        CHECK(rearranger.pop().has_value() == false);
    }

    SECTION("Multiple pushing and multiple popping")
    {
        trade::broker::Rearranger rearranger;

        rearranger.push(create_order(0));
        CHECK(rearranger.pop().value()->unique_id() == 0);
        CHECK(rearranger.pop().has_value() == false);

        rearranger.push(create_order(2));
        CHECK(rearranger.pop().has_value() == false);
        rearranger.push(create_order(1));
        CHECK(rearranger.pop().value()->unique_id() == 1);
        CHECK(rearranger.pop().value()->unique_id() == 2);
        CHECK(rearranger.pop().has_value() == false);

        rearranger.push(create_order(4));
        CHECK(rearranger.pop().has_value() == false);
        rearranger.push(create_order(3));
        rearranger.push(create_order(5));
        CHECK(rearranger.pop().value()->unique_id() == 3);
        CHECK(rearranger.pop().value()->unique_id() == 4);
        CHECK(rearranger.pop().value()->unique_id() == 5);
        CHECK(rearranger.pop().has_value() == false);
    }
}
