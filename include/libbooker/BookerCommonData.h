#pragma once

#include <third/liquibook/src/book/order_book.h>

#include "enums.pb.h"

namespace trade::broker
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

} // namespace trade::broker