#include "libbooker/MdValidator.h"

void trade::booker::MdValidator::l2_tick_generated(const L2TickPtr& l2_tick)
{
    m_traded_ask_unique_id[l2_tick->symbol()] = l2_tick->ask_unique_id();
    m_traded_bid_unique_id[l2_tick->symbol()] = l2_tick->bid_unique_id();
    m_traded_quantity[l2_tick->symbol()]      = l2_tick->quantity();
}

bool trade::booker::MdValidator::check(const TradeTickPtr& trade_tick) const
{
    if (!(m_traded_ask_unique_id.contains(trade_tick->symbol())
          && m_traded_bid_unique_id.contains(trade_tick->symbol())))
        return false;

    const auto& ask_buffer        = m_traded_ask_unique_id.at(trade_tick->symbol());
    const auto& bid_buffer        = m_traded_bid_unique_id.at(trade_tick->symbol());
    const auto& quantities_buffer = m_traded_quantity.at(trade_tick->symbol());

    return trade_tick->ask_unique_id() == ask_buffer
        && trade_tick->bid_unique_id() == bid_buffer
        && trade_tick->exec_quantity() == quantities_buffer;
}
