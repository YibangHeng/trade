#include "libreporter/ShmReporter.h"
#include "utilities/MakeAssignable.hpp"
#include "utilities/TimeHelper.hpp"

trade::reporter::ShmReporter::ShmReporter(std::shared_ptr<IReporter> outside)
    : AppBase("ShmReporter"), /// TODO: Make it configurable.
      m_md_shm(boost::interprocess::open_or_create, "trade_data", boost::interprocess::read_write),
      m_named_mutex(boost::interprocess::open_or_create, "trade_data_mutex"),
      m_outside(std::move(outside))
{
    m_md_shm.truncate(1 * GB);

    boost::interprocess::offset_t shm_size;
    m_md_shm.get_size(shm_size);

    logger->info("Created/Opened shared memory {} with size {}GB({} bytes)", m_md_shm.get_name(), shm_size / GB, shm_size);

    /// Map areas of shared memory with same size.
    m_md_region     = std::make_shared<boost::interprocess::mapped_region>(m_md_shm, boost::interprocess::read_write);

    m_shm_mate_info = static_cast<SMMateInfo*>(m_md_region->get_address());

    /// Assign pointers with start address.
    m_md_start   = reinterpret_cast<SMMarketData*>(m_shm_mate_info + 1);
    m_md_current = m_md_start;

    /// Set memory areas.
    memset(m_md_region->get_address(), 0, shm_size);

    logger->info("Divided shared memory areas to trade({})", m_md_region->get_address());
}

void trade::reporter::ShmReporter::md_trade_generated(const std::shared_ptr<types::MdTrade> md_trade)
{
    /// Check if shared memory is full.
    if (m_md_current - m_md_start > m_md_region->get_size() / sizeof(SMMarketData)) {
        logger->error("Shared memory of trade is full");
        return;
    }

    /// Writing scope.
    {
        boost::interprocess::scoped_lock lock(m_named_mutex);

        M_A {m_md_current->symbol} = md_trade->symbol();
        m_md_current->price        = md_trade->price();
        m_md_current->quantity     = static_cast<int32_t>(md_trade->quantity());

        m_md_current->sell_10      = md_trade->sell_10();
        m_md_current->sell_9       = md_trade->sell_9();
        m_md_current->sell_8       = md_trade->sell_8();
        m_md_current->sell_7       = md_trade->sell_7();
        m_md_current->sell_6       = md_trade->sell_6();
        m_md_current->sell_5       = md_trade->sell_5();
        m_md_current->sell_4       = md_trade->sell_4();
        m_md_current->sell_3       = md_trade->sell_3();
        m_md_current->sell_2       = md_trade->sell_2();
        m_md_current->sell_1       = md_trade->sell_1();
        m_md_current->buy_1        = md_trade->buy_1();
        m_md_current->buy_2        = md_trade->buy_2();
        m_md_current->buy_3        = md_trade->buy_3();
        m_md_current->buy_4        = md_trade->buy_4();
        m_md_current->buy_5        = md_trade->buy_5();
        m_md_current->buy_6        = md_trade->buy_6();
        m_md_current->buy_7        = md_trade->buy_7();
        m_md_current->buy_8        = md_trade->buy_8();
        m_md_current->buy_9        = md_trade->buy_9();
        m_md_current->buy_10       = md_trade->buy_10();

        m_md_current++;
        m_shm_mate_info->market_data_count++;
        M_A {m_shm_mate_info->last_update_time} = utilities::Now<std::string>()();
    }

    m_outside->md_trade_generated(md_trade);
}
