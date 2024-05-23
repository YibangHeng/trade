#include "libbroker/CUTImpl/OrderWrapper.h"

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
    return static_cast<liquibook::book::Price>(m_order->price() * 1000);
}

liquibook::book::Quantity trade::broker::OrderWrapper::order_qty() const
{
    return static_cast<liquibook::book::Quantity>(m_order->quantity());
}
