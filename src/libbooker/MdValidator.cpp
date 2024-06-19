#include "libbooker/MdValidator.h"

void trade::booker::MdValidator::l2_tick_generated(const std::shared_ptr<types::L2Tick>& l2_tick)
{
    new_l2_tick_buffer(l2_tick->symbol());

    m_l2_tick_buffers[l2_tick->symbol()].push_back(MdTradeHash()(*l2_tick));
}

bool trade::booker::MdValidator::check(const L2TickPtr& l2_tick) const
{
    if (!m_l2_tick_buffers.contains(l2_tick->symbol()))
        return false;

    const auto& buffer = m_l2_tick_buffers.at(l2_tick->symbol());

    return std::ranges::find(std::rbegin(buffer), std::rend(buffer), MdTradeHash()(*l2_tick)) != buffer.rend();
}

void trade::booker::MdValidator::new_l2_tick_buffer(const std::string& symbol)
{
    if (m_l2_tick_buffers.contains(symbol))
        return;

    m_l2_tick_buffers[symbol] = boost::circular_buffer<MdTradeHash::HashType>(m_buffer_size);
}