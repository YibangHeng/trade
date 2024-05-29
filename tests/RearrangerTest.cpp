#include <catch.hpp>

#include "libbooker/OrderWrapper.h"
#include "libbooker/Rearranger.h"
#include "utilities/OrderCreator.hpp"

TEST_CASE("In order pushing", "[Rearranger]")
{
    SECTION("Single pushing and popping")
    {
        trade::booker::Rearranger rearranger;

        rearranger.push(OrderCreator::order_wrapper(0));

        CHECK(rearranger.pop().value()->unique_id() == 0);
        CHECK(rearranger.pop().has_value() == false);
    }

    SECTION("Multiple pushing and popping")
    {
        trade::booker::Rearranger rearranger;

        rearranger.push(OrderCreator::order_wrapper(0));
        rearranger.push(OrderCreator::order_wrapper(1));
        rearranger.push(OrderCreator::order_wrapper(2));

        CHECK(rearranger.pop().value()->unique_id() == 0);
        CHECK(rearranger.pop().value()->unique_id() == 1);
        CHECK(rearranger.pop().value()->unique_id() == 2);
        CHECK(rearranger.pop().has_value() == false);
    }

    SECTION("Multiple pushing and multiple popping")
    {
        trade::booker::Rearranger rearranger;

        rearranger.push(OrderCreator::order_wrapper(0));
        CHECK(rearranger.pop().value()->unique_id() == 0);
        CHECK(rearranger.pop().has_value() == false);

        rearranger.push(OrderCreator::order_wrapper(1));
        rearranger.push(OrderCreator::order_wrapper(2));
        CHECK(rearranger.pop().value()->unique_id() == 1);
        CHECK(rearranger.pop().value()->unique_id() == 2);
        CHECK(rearranger.pop().has_value() == false);

        rearranger.push(OrderCreator::order_wrapper(3));
        rearranger.push(OrderCreator::order_wrapper(4));
        rearranger.push(OrderCreator::order_wrapper(5));
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
        trade::booker::Rearranger rearranger;

        rearranger.push(OrderCreator::order_wrapper(1));
        rearranger.push(OrderCreator::order_wrapper(0));
        rearranger.push(OrderCreator::order_wrapper(2));

        CHECK(rearranger.pop().value()->unique_id() == 0);
        CHECK(rearranger.pop().value()->unique_id() == 1);
        CHECK(rearranger.pop().value()->unique_id() == 2);
        CHECK(rearranger.pop().has_value() == false);
    }

    SECTION("Multiple pushing and multiple popping")
    {
        trade::booker::Rearranger rearranger;

        rearranger.push(OrderCreator::order_wrapper(0));
        CHECK(rearranger.pop().value()->unique_id() == 0);
        CHECK(rearranger.pop().has_value() == false);

        rearranger.push(OrderCreator::order_wrapper(2));
        CHECK(rearranger.pop().has_value() == false);
        rearranger.push(OrderCreator::order_wrapper(1));
        CHECK(rearranger.pop().value()->unique_id() == 1);
        CHECK(rearranger.pop().value()->unique_id() == 2);
        CHECK(rearranger.pop().has_value() == false);

        rearranger.push(OrderCreator::order_wrapper(4));
        CHECK(rearranger.pop().has_value() == false);
        rearranger.push(OrderCreator::order_wrapper(3));
        rearranger.push(OrderCreator::order_wrapper(5));
        CHECK(rearranger.pop().value()->unique_id() == 3);
        CHECK(rearranger.pop().value()->unique_id() == 4);
        CHECK(rearranger.pop().value()->unique_id() == 5);
        CHECK(rearranger.pop().has_value() == false);
    }
}
