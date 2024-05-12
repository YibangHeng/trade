#include <catch.hpp>

#include "libholder/SQLiteHolder.h"
#include "utilities/TimeHelper.hpp"

constexpr size_t insertion_times = 16;
constexpr size_t insertion_batch = 1024;

/// A global holder for testing.
auto holder = std::make_shared<trade::holder::SQLiteHolder>();

TEST_CASE("Funds inserting", "[SQLiteHolder]")
{
    int counter = 0;

    for (int i = 0; i < insertion_times; i++) {
        const auto funds = std::make_shared<trade::types::Funds>();

        for (int j = 0; j < insertion_batch; j++) {
            trade::types::Fund fund;

            fund.set_account_id(fmt::format("account_id_{}", counter));
            fund.set_available_fund(counter);
            fund.set_withdrawn_fund(counter);
            fund.set_frozen_fund(counter);
            fund.set_frozen_margin(counter);
            fund.set_frozen_commission(counter);

            counter++;

            funds->add_funds()->CopyFrom(fund);
        }

        CHECK(holder->update_funds(funds) == insertion_batch);
    }
}

TEST_CASE("Funds querying", "[SQLiteHolder]")
{
    int counter = 0;

    for (int i = 0; i < insertion_times * insertion_batch; i++) {
        const auto funds = holder->query_funds_by_account_id(fmt::format("account_id_{}", counter));

        REQUIRE(funds->funds_size() == 1);

        CHECK(funds->funds(0).account_id() == fmt::format("account_id_{}", counter));
        CHECK(funds->funds(0).available_fund() == counter);
        CHECK(funds->funds(0).frozen_fund() == counter);
        CHECK(funds->funds(0).frozen_margin() == counter);
        CHECK(funds->funds(0).frozen_commission() == counter);
        CHECK(funds->funds(0).withdrawn_fund() == counter);

        counter++;
    }
}

TEST_CASE("Position inserting", "[SQLiteHolder]")
{
    int counter = 0;

    for (int i = 0; i < insertion_times; i++) {
        const auto positions = std::make_shared<trade::types::Positions>();

        for (int j = 0; j < insertion_batch; j++) {
            trade::types::Position position;

            position.set_symbol(fmt::format("symbol_{}", counter));
            position.set_yesterday_position(counter);
            position.set_today_position(counter);
            position.set_open_volume(counter);
            position.set_close_volume(counter);
            position.set_position_cost(counter);
            position.set_pre_margin(counter);
            position.set_used_margin(counter);
            position.set_frozen_margin(counter);
            position.set_open_cost(counter);
#ifdef LIB_DATE_SUPPORT
            position.set_allocated_update_time(trade::utilities::ToTime<google::protobuf::Timestamp*>()(fmt::format("2000-01-01 08:00:00.{:0>3}", counter % 100)));
#endif

            counter++;

            positions->add_positions()->CopyFrom(position);
        }

        CHECK(holder->update_positions(positions) == insertion_batch);
    }
}

TEST_CASE("Position querying", "[SQLiteHolder]")
{
    int counter = 0;

    for (int i = 0; i < insertion_times * insertion_batch; i++) {
        const auto positions = holder->query_positions_by_symbol(fmt::format("symbol_{}", counter));

        REQUIRE(positions->positions_size() == 1);

        CHECK(positions->positions(0).symbol() == fmt::format("symbol_{}", counter));
        CHECK(positions->positions(0).yesterday_position() == counter);
        CHECK(positions->positions(0).today_position() == counter);
        CHECK(positions->positions(0).open_volume() == counter);
        CHECK(positions->positions(0).close_volume() == counter);
        CHECK(positions->positions(0).position_cost() == counter);
        CHECK(positions->positions(0).pre_margin() == counter);
        CHECK(positions->positions(0).used_margin() == counter);
        CHECK(positions->positions(0).frozen_margin() == counter);
        CHECK(positions->positions(0).open_cost() == counter);
#ifdef LIB_DATE_SUPPORT
        CHECK(trade::utilities::ToTime<std::string>()(positions->positions(0).update_time()) == fmt::format("2000-01-01 08:00:00.{:0>3}", counter % 100));
#endif

        counter++;
    }
}

TEST_CASE("Order inserting", "[SQLiteHolder]")
{
    /// An order with unique_id == INVALID_ID will be ignored, so we start from
    /// INVALID_ID + 1.
    int counter = INVALID_ID + 1;

    for (int i = 0; i < insertion_times; i++) {
        const auto orders = std::make_shared<trade::types::Orders>();

        for (int j = 0; j < insertion_batch; j++) {
            trade::types::Order order;

            order.set_unique_id(counter);
            order.set_broker_id(fmt::format("broker_id_{}", counter));
            order.set_exchange_id(fmt::format("exchange_id_{}", counter));
            order.set_symbol(fmt::format("symbol_{}", counter));
            order.set_side(trade::types::SideType::buy);
            order.set_position_side(trade::types::PositionSideType::open);
            order.set_price(counter);
            order.set_quantity(counter);
#ifdef LIB_DATE_SUPPORT
            order.set_allocated_creation_time(trade::utilities::ToTime<google::protobuf::Timestamp*>()("2000-01-01 08:00:00.000"));
            order.set_allocated_update_time(trade::utilities::ToTime<google::protobuf::Timestamp*>()(fmt::format("2000-01-01 08:00:00.{:0>3}", counter % 100)));
#endif

            counter++;

            orders->add_orders()->CopyFrom(order);
        }

        CHECK(holder->update_orders(orders) == insertion_batch);
    }
}

TEST_CASE("Order querying", "[SQLiteHolder]")
{
    /// An order with unique_id == INVALID_ID will be ignored, so we start from
    /// INVALID_ID + 1.
    int counter = INVALID_ID + 1;

    for (int i = 0; i < insertion_times * insertion_batch; i++) {
        const auto orders = holder->query_orders_by_unique_id(counter);

        REQUIRE(orders->orders_size() == 1);

        CHECK(orders->orders(0).unique_id() == counter);
        CHECK(orders->orders(0).broker_id() == fmt::format("broker_id_{}", counter));
        CHECK(orders->orders(0).exchange_id() == fmt::format("exchange_id_{}", counter));
        CHECK(orders->orders(0).symbol() == fmt::format("symbol_{}", counter));
        CHECK(orders->orders(0).side() == trade::types::SideType::buy);
        CHECK(orders->orders(0).position_side() == trade::types::PositionSideType::open);
        CHECK(orders->orders(0).price() == counter);
        CHECK(orders->orders(0).quantity() == counter);
#ifdef LIB_DATE_SUPPORT
        CHECK(trade::utilities::ToTime<std::string>()(orders->orders(0).creation_time()) == "2000-01-01 08:00:00.000");
        CHECK(trade::utilities::ToTime<std::string>()(orders->orders(0).update_time()) == fmt::format("2000-01-01 08:00:00.{:0>3}", counter % 100));
#endif

        counter++;
    }
}
