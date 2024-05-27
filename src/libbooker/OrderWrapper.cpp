#include "libbooker/OrderWrapper.h"
#include "libbooker/BookerCommonData.h"

trade::broker::OrderWrapper::OrderWrapper(const std::shared_ptr<types::OrderTick>& order_tick)
{
    this->m_order = order_tick;
}

int64_t trade::broker::OrderWrapper::unique_id() const
{
    return m_order->unique_id();
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

bool trade::broker::OrderWrapper::is_limit() const
{
    return m_order->order_type() == types::OrderType::limit;
}
