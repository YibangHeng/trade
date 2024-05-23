#include "libbooker/BookerCommonData.h"

char trade::broker::BookerCommonData::to_side(const types::SideType side)
{
    switch (side) {
    case types::SideType::buy: return '1';
    case types::SideType::sell: return '2';
    default: return '\0';
    }
}

trade::types::SideType trade::broker::BookerCommonData::to_side(const char side)
{
    switch (side) {
    case '1': return types::SideType::buy;
    case '2': return types::SideType::sell;
    default: return types::SideType::invalid_side;
    }
}

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