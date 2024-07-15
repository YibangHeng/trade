#include <catch.hpp>

#include "libbooker/CallAuctionHolder.h"
#include "libbooker/OrderWrapper.h"
#include "utilities/OrderCreator.hpp"

TEST_CASE("Call auction logic test", "[CallAuctionHolder]")
{
    SECTION("No trade")
    {
        trade::booker::CallAuctionHolder call_auction_holder;

        call_auction_holder.push(TickCreator::order_wrapper(0, LIMIT, "600875.SH", BUY, 22.33, 100));
        call_auction_holder.push(TickCreator::order_wrapper(1, LIMIT, "600875.SH", BUY, 22.33, 100));
        call_auction_holder.push(TickCreator::order_wrapper(2, LIMIT, "600875.SH", BUY, 22.33, 100));
        call_auction_holder.push(TickCreator::order_wrapper(3, LIMIT, "600875.SH", SELL, 22.33, 100));
        call_auction_holder.push(TickCreator::order_wrapper(4, LIMIT, "600875.SH", SELL, 22.33, 100));
        call_auction_holder.push(TickCreator::order_wrapper(5, LIMIT, "600875.SH", SELL, 22.33, 100));

        auto order = call_auction_holder.pop();
        CHECK(order->unique_id() == 0);
        CHECK(order->quantity() == 100);

        order = call_auction_holder.pop();
        CHECK(order->unique_id() == 1);
        CHECK(order->quantity() == 100);

        order = call_auction_holder.pop();
        CHECK(order->unique_id() == 2);
        CHECK(order->quantity() == 100);

        order = call_auction_holder.pop();
        CHECK(order->unique_id() == 3);
        CHECK(order->quantity() == 100);

        order = call_auction_holder.pop();
        CHECK(order->unique_id() == 4);
        CHECK(order->quantity() == 100);

        order = call_auction_holder.pop();
        CHECK(order->unique_id() == 5);
        CHECK(order->quantity() == 100);

        order = call_auction_holder.pop();
        CHECK(order == nullptr);
    }

    SECTION("Half trade")
    {
        trade::booker::CallAuctionHolder call_auction_holder;

        call_auction_holder.push(TickCreator::order_wrapper(0, LIMIT, "600875.SH", BUY, 22.33, 100));
        call_auction_holder.push(TickCreator::order_wrapper(1, LIMIT, "600875.SH", BUY, 22.33, 100));
        call_auction_holder.push(TickCreator::order_wrapper(2, LIMIT, "600875.SH", BUY, 22.33, 100));
        call_auction_holder.push(TickCreator::order_wrapper(3, LIMIT, "600875.SH", SELL, 22.33, 100));
        call_auction_holder.push(TickCreator::order_wrapper(4, LIMIT, "600875.SH", SELL, 22.33, 100));
        call_auction_holder.push(TickCreator::order_wrapper(5, LIMIT, "600875.SH", SELL, 22.33, 100));

        call_auction_holder.trade(*TickCreator::trade_tick(3, 0, "600875.SH", 22.33, 50, 925000));
        call_auction_holder.trade(*TickCreator::trade_tick(4, 1, "600875.SH", 22.33, 50, 925000));
        call_auction_holder.trade(*TickCreator::trade_tick(5, 2, "600875.SH", 22.33, 50, 925000));

        auto order = call_auction_holder.pop();
        CHECK(order->unique_id() == 0);
        CHECK(order->quantity() == 50);

        order = call_auction_holder.pop();
        CHECK(order->unique_id() == 1);
        CHECK(order->quantity() == 50);

        order = call_auction_holder.pop();
        CHECK(order->unique_id() == 2);
        CHECK(order->quantity() == 50);

        order = call_auction_holder.pop();
        CHECK(order->unique_id() == 3);
        CHECK(order->quantity() == 50);

        order = call_auction_holder.pop();
        CHECK(order->unique_id() == 4);
        CHECK(order->quantity() == 50);

        order = call_auction_holder.pop();
        CHECK(order->unique_id() == 5);
        CHECK(order->quantity() == 50);

        order = call_auction_holder.pop();
        CHECK(order == nullptr);
    }

    SECTION("Full trade")
    {
        trade::booker::CallAuctionHolder call_auction_holder;

        call_auction_holder.push(TickCreator::order_wrapper(0, LIMIT, "600875.SH", BUY, 22.33, 100));
        call_auction_holder.push(TickCreator::order_wrapper(1, LIMIT, "600875.SH", BUY, 22.33, 100));
        call_auction_holder.push(TickCreator::order_wrapper(2, LIMIT, "600875.SH", BUY, 22.33, 100));
        call_auction_holder.push(TickCreator::order_wrapper(3, LIMIT, "600875.SH", SELL, 22.33, 100));
        call_auction_holder.push(TickCreator::order_wrapper(4, LIMIT, "600875.SH", SELL, 22.33, 100));
        call_auction_holder.push(TickCreator::order_wrapper(5, LIMIT, "600875.SH", SELL, 22.33, 100));

        call_auction_holder.trade(*TickCreator::trade_tick(3, 0, "600875.SH", 22.33, 100, 925000));
        call_auction_holder.trade(*TickCreator::trade_tick(4, 1, "600875.SH", 22.33, 100, 925000));
        call_auction_holder.trade(*TickCreator::trade_tick(5, 2, "600875.SH", 22.33, 100, 925000));

        auto order = call_auction_holder.pop();
        CHECK(order == nullptr);
    }
}
