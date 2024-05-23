#include "libbooker/BookerCommonData.h"

liquibook::book::Price trade::broker::BookerCommonData::to_price(const double price)
{
    return static_cast<liquibook::book::Price>(price * scale);
}

double trade::broker::BookerCommonData::to_price(const liquibook::book::Price price)
{
    return static_cast<double>(price) / scale;
}

liquibook::book::Quantity trade::broker::BookerCommonData::to_quantity(const int64_t quantity)
{
    return static_cast<liquibook::book::Quantity>(quantity * scale);
}

int64_t trade::broker::BookerCommonData::to_quantity(const liquibook::book::Quantity quantity)
{
    return static_cast<int64_t>(quantity / scale);
}