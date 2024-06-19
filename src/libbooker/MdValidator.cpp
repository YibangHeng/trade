#include "libbooker/MdValidator.h"

void trade::booker::MdValidator::md_trade_generated(const std::shared_ptr<types::MdTrade>& md_trade)
{
    new_md_trade_buffer(md_trade->symbol());

    m_md_trade_buffers[md_trade->symbol()].push_back(MdTradeHash()(*md_trade));
}

bool trade::booker::MdValidator::check(const MdTradePtr& md_trade) const
{
    if (!m_md_trade_buffers.contains(md_trade->symbol()))
        return false;

    const auto& buffer = m_md_trade_buffers.at(md_trade->symbol());

    return std::ranges::find(std::rbegin(buffer), std::rend(buffer), MdTradeHash()(*md_trade)) != buffer.rend();
}

void trade::booker::MdValidator::new_md_trade_buffer(const std::string& symbol)
{
    if (m_md_trade_buffers.contains(symbol))
        return;

    m_md_trade_buffers[symbol] = boost::circular_buffer<MdTradeHash::HashType>(m_buffer_size);
}