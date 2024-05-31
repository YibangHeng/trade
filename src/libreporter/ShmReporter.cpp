#include "libreporter/ShmReporter.h"
#include "utilities/MakeAssignable.hpp"

trade::reporter::ShmReporter::ShmReporter(std::shared_ptr<IReporter> outside)
    : AppBase("ShmReporter"), /// TODO: Make it configurable.
      m_shm(boost::interprocess::open_or_create, "trade_data", boost::interprocess::read_write),
      m_outside(std::move(outside))
{
    m_shm.truncate(3 * GB);

    boost::interprocess::offset_t shm_size;
    m_shm.get_size(shm_size);

    logger->info("Created/Opened shared memory {} with size {}GB({} bytes)", m_shm.get_name(), shm_size / GB, shm_size);

    /// Map areas of shared memory with same size.
    m_trade_region        = std::make_shared<boost::interprocess::mapped_region>(m_shm, boost::interprocess::read_write, 0 * shm_size / 3, shm_size / 3);
    m_market_price_region = std::make_shared<boost::interprocess::mapped_region>(m_shm, boost::interprocess::read_write, 1 * shm_size / 3, shm_size / 3);
    m_level_price_region  = std::make_shared<boost::interprocess::mapped_region>(m_shm, boost::interprocess::read_write, 2 * shm_size / 3, shm_size / 3);

    /// Assign pointers with start address.
    m_trade_start        = static_cast<SMTrade*>(m_trade_region->get_address());
    m_market_price_start = static_cast<SMMarketPrice*>(m_market_price_region->get_address());
    m_level_price_start  = static_cast<SMLevelPrice*>(m_level_price_region->get_address());

    /// Set memory areas.
    memset(m_trade_start, 0, shm_size / 3);
    memset(m_market_price_start, 0, shm_size / 3);
    memset(m_level_price_start, 0, shm_size / 3);

    logger->info("Divided shared memory areas to trade({}), market_price({}) and level_price({})", m_trade_region->get_address(), m_market_price_region->get_address(), m_level_price_region->get_address());
}

void trade::reporter::ShmReporter::md_trade_generated(const std::shared_ptr<types::MdTrade> md_trade)
{
    static auto trade_p = m_trade_start;

    /// Check if shared memory is full.
    if (trade_p - m_trade_start > m_trade_region->get_size() / sizeof(SMTrade)) {
        logger->error("Shared memory of trade is full");
        return;
    }

    SMTrade sm_trade;

    M_A {sm_trade.symbol} = md_trade->symbol();
    sm_trade.price        = md_trade->price();
    sm_trade.quantity     = static_cast<int32_t>(md_trade->quantity());

    memcpy(trade_p, &sm_trade, sizeof(sm_trade));
    trade_p++;

    m_outside->md_trade_generated(md_trade);
}

void trade::reporter::ShmReporter::market_price(std::string symbol, double price)
{
}

void trade::reporter::ShmReporter::level_price(std::array<double, level_depth> asks, std::array<double, level_depth> bids)
{
}
