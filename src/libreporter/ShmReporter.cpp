#include "libreporter/ShmReporter.h"
#include "utilities/MakeAssignable.hpp"

trade::reporter::ShmReporter::ShmReporter(std::shared_ptr<IReporter> outside)
    : AppBase("ShmReporter"), /// TODO: Make it configurable.
      m_md_shm(boost::interprocess::open_or_create, "trade_data", boost::interprocess::read_write),
      m_outside(std::move(outside))
{
    m_md_shm.truncate(1 * GB);

    boost::interprocess::offset_t shm_size;
    m_md_shm.get_size(shm_size);

    logger->info("Created/Opened shared memory {} with size {}GB({} bytes)", m_md_shm.get_name(), shm_size / GB, shm_size);

    /// Map areas of shared memory with same size.
    m_md_region = std::make_shared<boost::interprocess::mapped_region>(m_md_shm, boost::interprocess::read_write);

    /// Assign pointers with start address.
    m_md_start = static_cast<SMMarketData*>(m_md_region->get_address());

    /// Set memory areas.
    memset(m_md_start, 0, shm_size);

    logger->info("Divided shared memory areas to trade({})", m_md_region->get_address());
}

void trade::reporter::ShmReporter::md_trade_generated(const std::shared_ptr<types::MdTrade> md_trade)
{
    static auto trade_p = m_md_start;

    /// Check if shared memory is full.
    if (trade_p - m_md_start > m_md_region->get_size() / sizeof(SMMarketData)) {
        logger->error("Shared memory of trade is full");
        return;
    }

    SMMarketData sm_trade;

    M_A {sm_trade.symbol} = md_trade->symbol();
    sm_trade.price        = md_trade->price();
    sm_trade.quantity     = static_cast<int32_t>(md_trade->quantity());

    memcpy(trade_p, &sm_trade, sizeof(sm_trade));
    trade_p++;

    m_outside->md_trade_generated(md_trade);
}
