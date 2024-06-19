#pragma once

#include <third/liquibook/src/book/order_book.h>

#include "enums.pb.h"
#include "networks.pb.h"
#include "orms.pb.h"

namespace trade::booker
{

class BookerCommonData
{
public:
    [[nodiscard]] static char to_side(types::SideType side);
    [[nodiscard]] static types::SideType to_side(char side);
    [[nodiscard]] static liquibook::book::Price to_price(double price);
    [[nodiscard]] static double to_price(liquibook::book::Price price);
    [[nodiscard]] static liquibook::book::Quantity to_quantity(int64_t quantity);
    [[nodiscard]] static int64_t to_quantity(liquibook::book::Quantity quantity);

private:
    static constexpr int64_t scale = 1000;
};

using OrderTickPtr = std::shared_ptr<types::OrderTick>;
using TradeTickPtr = std::shared_ptr<types::TradeTick>;
using L2TickPtr    = std::shared_ptr<types::L2Tick>;

} // namespace trade::booker
