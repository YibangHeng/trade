#include "libbooker/OrderWrapper.h"
#include "libbooker/BookerCommonData.h"

trade::booker::OrderWrapper::OrderWrapper(const OrderTickPtr& order_tick)
    : filled_quantity(0)
{
    this->m_order = order_tick;
}

int64_t trade::booker::OrderWrapper::unique_id() const
{
    return m_order->unique_id();
}

void trade::booker::OrderWrapper::to_limit_order(const int64_t price) const
{
    assert(m_order->order_type() == types::OrderType::market || m_order->order_type() == types::OrderType::best_price);

    m_order->set_order_type(types::OrderType::limit);
    m_order->set_price_1000x(price);
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
    return BookerCommonData::to_price(m_order->price_1000x());
}

liquibook::book::Quantity trade::booker::OrderWrapper::order_qty() const
{
    return BookerCommonData::to_quantity(m_order->quantity());
}

bool trade::booker::OrderWrapper::is_limit() const
{
    return order_type() == types::OrderType::limit;
}

bool trade::booker::OrderWrapper::accept(const types::TradeTick& trade_tick)
{
    return (filled_quantity += BookerCommonData::to_quantity(trade_tick.exec_quantity()))
        >= BookerCommonData::to_quantity(m_order->quantity());
}

trade::types::OrderType trade::booker::OrderWrapper::order_type() const
{
    return m_order->order_type();
}

int64_t trade::booker::OrderWrapper::exchange_date() const
{
    return m_order->exchange_date();
}

int64_t trade::booker::OrderWrapper::exchange_time() const
{
    return m_order->exchange_time();
}

liquibook::book::Quantity trade::booker::OrderWrapper::quantity_on_market() const
{
    return BookerCommonData::to_quantity(m_order->quantity()) - filled_quantity;
}

trade::booker::OrderTickPtr trade::booker::OrderWrapper::raw_order_tick() const
{
    return m_order;
}

void trade::booker::OrderWrapper::mark_as_cancel(const int64_t exchange_time) const
{
    m_order->set_order_type(types::OrderType::cancel);
    m_order->set_exchange_time(exchange_time);
}
