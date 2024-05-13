#include <catch.hpp>

#include "libholder/SQLiteHolder.h"
#include "utilities/TimeHelper.hpp"

constexpr size_t insertion_times = 16;
constexpr size_t insertion_batch = 1024;

/// A global holder for testing.
auto holder = std::make_shared<trade::holder::SQLiteHolder>();

/// All optional fields of odd rows will be null.

TEST_CASE("Symbols inserting", "[SQLiteHolder]")
{
    int counter = 0;

    for (int i = 0; i < insertion_times; i++) {
        const auto symbols = std::make_shared<trade::types::Symbols>();

        for (int j = 0; j < insertion_batch; j++) {
            trade::types::Symbol symbol;

            symbol.set_symbol(fmt::format("symbol_{}", counter));
            symbol.set_symbol_name(fmt::format("symbol_name_{}", counter));
            symbol.set_exchange(j % 2 ? trade::types::ExchangeType::sse : trade::types::ExchangeType::szse);
            if (j % 2 == 0)
                symbol.set_underlying(fmt::format("underlying_{}", counter));

            counter++;

            symbols->add_symbols()->CopyFrom(symbol);
        }

        CHECK(holder->update_symbols(symbols) == insertion_batch);
    }
}

TEST_CASE("Symbols querying by symbol", "[SQLiteHolder]")
{
    int counter = 0;

    for (int i = 0; i < insertion_times * insertion_batch; i++) {
        const auto symbols = holder->query_symbols_by_symbol(fmt::format("symbol_{}", counter));

        REQUIRE(symbols->symbols_size() == 1);

        CHECK(symbols->symbols(0).symbol() == fmt::format("symbol_{}", counter));
        CHECK(symbols->symbols(0).symbol_name() == fmt::format("symbol_name_{}", counter));
        CHECK(symbols->symbols(0).exchange() == (counter % 2 ? trade::types::ExchangeType::sse : trade::types::ExchangeType::szse));
        if (counter % 2 == 0)
            CHECK(symbols->symbols(0).underlying() == fmt::format("underlying_{}", counter));
        else
            CHECK(symbols->symbols(0).has_underlying() == false);

        counter++;
    }
}

TEST_CASE("Symbols querying by exchange", "[SQLiteHolder]")
{
    const auto symbols_in_sse = holder->query_symbols_by_exchange(trade::types::ExchangeType::sse);

    REQUIRE(symbols_in_sse->symbols_size() == insertion_times * insertion_batch / 2);

    for (const auto& symbol : symbols_in_sse->symbols()) {
        CHECK(symbol.exchange() == trade::types::ExchangeType::sse);
    }

    const auto symbols_in_szse = holder->query_symbols_by_exchange(trade::types::ExchangeType::szse);

    REQUIRE(symbols_in_szse->symbols_size() == insertion_times * insertion_batch / 2);

    for (const auto& symbol : symbols_in_szse->symbols()) {
        CHECK(symbol.exchange() == trade::types::ExchangeType::szse);
    }
}

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
            if (counter % 2 == 0) {
                position.set_open_volume(counter);
                position.set_close_volume(counter);
                position.set_position_cost(counter);
                position.set_pre_margin(counter);
                position.set_used_margin(counter);
                position.set_frozen_margin(counter);
                position.set_open_cost(counter);
            }
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
        if (counter % 2 == 0) {
            CHECK(positions->positions(0).open_volume() == counter);
            CHECK(positions->positions(0).close_volume() == counter);
            CHECK(positions->positions(0).position_cost() == counter);
            CHECK(positions->positions(0).pre_margin() == counter);
            CHECK(positions->positions(0).used_margin() == counter);
            CHECK(positions->positions(0).frozen_margin() == counter);
            CHECK(positions->positions(0).open_cost() == counter);
        }
        else {
            CHECK(positions->positions(0).has_open_volume() == false);
            CHECK(positions->positions(0).has_close_volume() == false);
            CHECK(positions->positions(0).has_position_cost() == false);
            CHECK(positions->positions(0).has_pre_margin() == false);
            CHECK(positions->positions(0).has_used_margin() == false);
            CHECK(positions->positions(0).has_frozen_margin() == false);
            CHECK(positions->positions(0).has_open_cost() == false);
        }
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
            if (counter % 2 == 0) {
                order.set_broker_id(fmt::format("broker_id_{}", counter));
                order.set_exchange_id(fmt::format("exchange_id_{}", counter));
            }
            order.set_symbol(fmt::format("symbol_{}", counter));
            order.set_side(trade::types::SideType::buy);
            if (counter % 2 == 0)
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

TEST_CASE("Order querying by unique id", "[SQLiteHolder]")
{
    /// An order with unique_id == INVALID_ID will be ignored, so we start from
    /// INVALID_ID + 1.
    int counter = INVALID_ID + 1;

    for (int i = 0; i < insertion_times * insertion_batch; i++) {
        const auto orders = holder->query_orders_by_unique_id(counter);

        REQUIRE(orders->orders_size() == 1);

        CHECK(orders->orders(0).unique_id() == counter);
        if (counter % 2 == 0) {
            CHECK(orders->orders(0).broker_id() == fmt::format("broker_id_{}", counter));
            CHECK(orders->orders(0).exchange_id() == fmt::format("exchange_id_{}", counter));
        }
        else {
            CHECK(orders->orders(0).has_broker_id() == false);
            CHECK(orders->orders(0).has_exchange_id() == false);
        }
        CHECK(orders->orders(0).symbol() == fmt::format("symbol_{}", counter));
        CHECK(orders->orders(0).side() == trade::types::SideType::buy);
        if (counter % 2 == 0)
            CHECK(orders->orders(0).position_side() == trade::types::PositionSideType::open);
        else
            CHECK(orders->orders(0).has_position_side() == false);
        CHECK(orders->orders(0).price() == counter);
        CHECK(orders->orders(0).quantity() == counter);
#ifdef LIB_DATE_SUPPORT
        CHECK(trade::utilities::ToTime<std::string>()(orders->orders(0).creation_time()) == "2000-01-01 08:00:00.000");
        CHECK(trade::utilities::ToTime<std::string>()(orders->orders(0).update_time()) == fmt::format("2000-01-01 08:00:00.{:0>3}", counter % 100));
#endif

        counter++;
    }
}

TEST_CASE("Order querying by broker id", "[SQLiteHolder]")
{
    /// An order with unique_id == INVALID_ID will be ignored, so we start from
    /// INVALID_ID + 1.
    int counter = INVALID_ID + 1;

    for (int i = 0; i < insertion_times * insertion_batch; i++) {
        const auto orders = holder->query_orders_by_broker_id(fmt::format("broker_id_{}", counter));

        if (counter % 2 == 0) {
            REQUIRE(orders->orders_size() == 1);
        }
        else {
            REQUIRE(orders->orders_size() == 0);
            continue;
        }

        CHECK(orders->orders(0).unique_id() == counter);
        if (counter % 2 == 0) {
            CHECK(orders->orders(0).broker_id() == fmt::format("broker_id_{}", counter));
            CHECK(orders->orders(0).exchange_id() == fmt::format("exchange_id_{}", counter));
        }
        else {
            CHECK(orders->orders(0).has_broker_id() == false);
            CHECK(orders->orders(0).has_exchange_id() == false);
        }
        CHECK(orders->orders(0).symbol() == fmt::format("symbol_{}", counter));
        CHECK(orders->orders(0).side() == trade::types::SideType::buy);
        if (counter % 2 == 0)
            CHECK(orders->orders(0).position_side() == trade::types::PositionSideType::open);
        else
            CHECK(orders->orders(0).has_position_side() == false);
        CHECK(orders->orders(0).price() == counter);
        CHECK(orders->orders(0).quantity() == counter);
#ifdef LIB_DATE_SUPPORT
        CHECK(trade::utilities::ToTime<std::string>()(orders->orders(0).creation_time()) == "2000-01-01 08:00:00.000");
        CHECK(trade::utilities::ToTime<std::string>()(orders->orders(0).update_time()) == fmt::format("2000-01-01 08:00:00.{:0>3}", counter % 100));
#endif

        counter++;
    }
}

TEST_CASE("Order querying by exchange id", "[SQLiteHolder]")
{
    /// An order with unique_id == INVALID_ID will be ignored, so we start from
    /// INVALID_ID + 1.
    int counter = INVALID_ID + 1;

    for (int i = 0; i < insertion_times * insertion_batch; i++) {
        const auto orders = holder->query_orders_by_exchange_id(fmt::format("exchange_id_{}", counter));

        if (counter % 2 == 0) {
            REQUIRE(orders->orders_size() == 1);
        }
        else {
            REQUIRE(orders->orders_size() == 0);
            continue;
        }

        CHECK(orders->orders(0).unique_id() == counter);
        if (counter % 2 == 0) {
            CHECK(orders->orders(0).broker_id() == fmt::format("broker_id_{}", counter));
            CHECK(orders->orders(0).exchange_id() == fmt::format("exchange_id_{}", counter));
        }
        else {
            CHECK(orders->orders(0).has_broker_id() == false);
            CHECK(orders->orders(0).has_exchange_id() == false);
        }
        CHECK(orders->orders(0).symbol() == fmt::format("symbol_{}", counter));
        CHECK(orders->orders(0).side() == trade::types::SideType::buy);
        if (counter % 2 == 0)
            CHECK(orders->orders(0).position_side() == trade::types::PositionSideType::open);
        else
            CHECK(orders->orders(0).has_position_side() == false);
        CHECK(orders->orders(0).price() == counter);
        CHECK(orders->orders(0).quantity() == counter);
#ifdef LIB_DATE_SUPPORT
        CHECK(trade::utilities::ToTime<std::string>()(orders->orders(0).creation_time()) == "2000-01-01 08:00:00.000");
        CHECK(trade::utilities::ToTime<std::string>()(orders->orders(0).update_time()) == fmt::format("2000-01-01 08:00:00.{:0>3}", counter % 100));
#endif

        counter++;
    }
}
