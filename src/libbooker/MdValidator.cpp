#include "libbooker/MdValidator.h"

void trade::booker::MdValidator::l2_tick_generated(const L2TickPtr& l2_tick)
{
    new_trade_tick_buffer(l2_tick->symbol());
    new_l2_tick_buffer(l2_tick->symbol());

    m_traded_ask_unique_ids[l2_tick->symbol()].push_back(l2_tick->ask_unique_id());
    m_traded_bid_unique_ids[l2_tick->symbol()].push_back(l2_tick->bid_unique_id());

    m_l2_tick_buffers[l2_tick->symbol()].push_back(L2TickHash()(*l2_tick));
}

bool trade::booker::MdValidator::check(const TradeTickPtr& trade_tick) const
{
    if (!(m_traded_ask_unique_ids.contains(trade_tick->symbol())
          && m_traded_bid_unique_ids.contains(trade_tick->symbol())))
        return true;

    const auto& ask_buffer = m_traded_ask_unique_ids.at(trade_tick->symbol());
    const auto& bid_buffer = m_traded_bid_unique_ids.at(trade_tick->symbol());

    return std::ranges::find(std::rbegin(ask_buffer), std::rend(ask_buffer), trade_tick->ask_unique_id()) != ask_buffer.rend()
        && std::ranges::find(std::rbegin(bid_buffer), std::rend(bid_buffer), trade_tick->bid_unique_id()) != bid_buffer.rend();
}

bool trade::booker::MdValidator::check(const L2TickPtr& l2_tick) const
{
    if (!m_l2_tick_buffers.contains(l2_tick->symbol()))
        return true;

    const auto& buffer = m_l2_tick_buffers.at(l2_tick->symbol());

    return std::ranges::find(std::rbegin(buffer), std::rend(buffer), L2TickHash()(*l2_tick)) != buffer.rend();
}

void trade::booker::MdValidator::new_trade_tick_buffer(const std::string& symbol)
{
    if (!m_traded_ask_unique_ids.contains(symbol))
        m_traded_ask_unique_ids[symbol] = boost::circular_buffer<int64_t>(m_buffer_size);

    if (!m_traded_bid_unique_ids.contains(symbol))
        m_traded_bid_unique_ids[symbol] = boost::circular_buffer<int64_t>(m_buffer_size);
}

void trade::booker::MdValidator::new_l2_tick_buffer(const std::string& symbol)
{
    if (m_l2_tick_buffers.contains(symbol))
        return;

    m_l2_tick_buffers[symbol] = boost::circular_buffer<L2TickHash::HashType>(m_buffer_size);
}