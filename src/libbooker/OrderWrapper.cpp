#include "libbooker/OrderWrapper.h"
#include "libbooker/BookerCommonData.h"

trade::booker::OrderWrapper::OrderWrapper(const std::shared_ptr<types::OrderTick>& order_tick)
{
    this->m_order = order_tick;
}

int64_t trade::booker::OrderWrapper::unique_id() const
{
    return m_order->unique_id();
}

void trade::booker::OrderWrapper::to_limit_order(const double price) const
{
    assert(m_order->order_type() == types::OrderType::best_price);

    m_order->set_order_type(types::OrderType::limit);
    m_order->set_price(price);
}

std::string trade::booker::OrderWrapper::symbol() const
{
    return m_order->symbol();
}

bool trade::booker::OrderWrapper::is_buy() const
{
    return m_order->side() == types::SideType::buy;
}

liquibook::book::Price trade::booker::OrderWrapper::price() const
{
    return BookerCommonData::to_price(m_order->price());
}

liquibook::book::Quantity trade::booker::OrderWrapper::order_qty() const
{
    return BookerCommonData::to_quantity(m_order->quantity());
}

bool trade::booker::OrderWrapper::is_limit() const
{
    return order_type() == types::OrderType::limit;
}

trade::types::OrderType trade::booker::OrderWrapper::order_type() const
{
    return m_order->order_type();
}

bool trade::booker::OrderWrapper::exchange_time() const
{
    return m_order->exchange_time();
}
