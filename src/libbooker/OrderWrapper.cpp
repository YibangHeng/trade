#include "libbooker/OrderWrapper.h"
#include "libbooker/BookerCommonData.h"

trade::broker::OrderWrapper::OrderWrapper(const std::shared_ptr<types::OrderTick>& order_tick)
{
    this->m_order = order_tick;
}

std::string trade::broker::OrderWrapper::symbol() const
{
    return m_order->symbol();
}

bool trade::broker::OrderWrapper::is_buy() const
{
    return m_order->side() == types::SideType::buy;
}

liquibook::book::Price trade::broker::OrderWrapper::price() const
{
    return BookerCommonData::to_price(m_order->price());
}

liquibook::book::Quantity trade::broker::OrderWrapper::order_qty() const
{
    return BookerCommonData::to_quantity(m_order->quantity());
}
