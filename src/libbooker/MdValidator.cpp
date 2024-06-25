#include "libbooker/MdValidator.h"

void trade::booker::MdValidator::l2_tick_generated(const L2TickPtr& l2_tick)
{
    new_trade_tick_buffer(l2_tick->symbol());
    new_l2_tick_buffer(l2_tick->symbol());

    m_trade_tick_buffers[l2_tick->symbol()].push_back(TradeTickHash()(*l2_tick));
    m_l2_tick_buffers[l2_tick->symbol()].push_back(L2TickHash()(*l2_tick));
}

bool trade::booker::MdValidator::check(const TradeTickPtr& trade_tick) const
{
    if (!m_trade_tick_buffers.contains(trade_tick->symbol()))
        return false;

    const auto& buffer = m_trade_tick_buffers.at(trade_tick->symbol());

    return std::ranges::find(std::rbegin(buffer), std::rend(buffer), TradeTickHash()(*trade_tick)) != buffer.rend();
}

bool trade::booker::MdValidator::check(const L2TickPtr& l2_tick) const
{
    if (!m_l2_tick_buffers.contains(l2_tick->symbol()))
        return false;

    const auto& buffer = m_l2_tick_buffers.at(l2_tick->symbol());

    return std::ranges::find(std::rbegin(buffer), std::rend(buffer), L2TickHash()(*l2_tick)) != buffer.rend();
}

void trade::booker::MdValidator::new_trade_tick_buffer(const std::string& symbol)
{
    if (m_trade_tick_buffers.contains(symbol))
        return;

    m_trade_tick_buffers[symbol] = boost::circular_buffer<TradeTickHash::HashType>(m_buffer_size);
}

void trade::booker::MdValidator::new_l2_tick_buffer(const std::string& symbol)
{
    if (m_l2_tick_buffers.contains(symbol))
        return;

    m_l2_tick_buffers[symbol] = boost::circular_buffer<L2TickHash::HashType>(m_buffer_size);
}