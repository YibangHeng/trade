#include "libreporter/ShmReporter.h"
#include "utilities/MakeAssignable.hpp"
#include "utilities/TimeHelper.hpp"

trade::reporter::ShmReporter::ShmReporter(
    const std::string& shm_name,
    const std::string& shm_mutex_name,
    const int shm_size,
    std::shared_ptr<IReporter> outside
)
    : AppBase("ShmReporter"),
      m_md_shm(boost::interprocess::open_or_create, shm_name.c_str(), boost::interprocess::read_write),
      m_named_mutex(boost::interprocess::open_or_create, shm_mutex_name.c_str()),
      m_outside(std::move(outside))
{
    m_md_shm.truncate(shm_size * GB);

    boost::interprocess::offset_t allocated_shm_size;
    m_md_shm.get_size(allocated_shm_size);

    logger->info("Created/Opened shared memory {} with size {}GB({} bytes)", m_md_shm.get_name(), allocated_shm_size / GB, allocated_shm_size);

    /// Map areas of shared memory with same size.
    m_md_region     = std::make_shared<boost::interprocess::mapped_region>(m_md_shm, boost::interprocess::read_write);

    m_shm_mate_info = static_cast<SMMateInfo*>(m_md_region->get_address());

    /// Assign pointers with start address.
    m_md_start   = reinterpret_cast<SMMarketData*>(m_shm_mate_info + 1);
    m_md_current = m_md_start;

    /// Set memory areas.
    memset(m_md_region->get_address(), 0, allocated_shm_size);

    logger->info("Divided shared memory areas to trade({})", m_md_region->get_address());
}

void trade::reporter::ShmReporter::md_trade_generated(const std::shared_ptr<types::MdTrade> md_trade)
{
    /// Check if shared memory is full.
    if (m_md_current - m_md_start > m_md_region->get_size() / sizeof(SMMarketData)) {
        logger->error("Shared memory of trade is full");
        m_outside->md_trade_generated(md_trade);
        return;
    }

    /// Writing scope.
    {
        boost::interprocess::scoped_lock lock(m_named_mutex);

        M_A {m_md_current->symbol}     = md_trade->symbol();
        m_md_current->price            = md_trade->price();
        m_md_current->quantity         = static_cast<int32_t>(md_trade->quantity());

        m_md_current->sell_10.price    = md_trade->sell_price_10();
        m_md_current->sell_10.quantity = md_trade->sell_quantity_10();
        m_md_current->sell_9.price     = md_trade->sell_price_9();
        m_md_current->sell_9.quantity  = md_trade->sell_quantity_9();
        m_md_current->sell_8.price     = md_trade->sell_price_8();
        m_md_current->sell_8.quantity  = md_trade->sell_quantity_8();
        m_md_current->sell_7.price     = md_trade->sell_price_7();
        m_md_current->sell_7.quantity  = md_trade->sell_quantity_7();
        m_md_current->sell_6.price     = md_trade->sell_price_6();
        m_md_current->sell_6.quantity  = md_trade->sell_quantity_6();
        m_md_current->sell_5.price     = md_trade->sell_price_5();
        m_md_current->sell_5.quantity  = md_trade->sell_quantity_5();
        m_md_current->sell_4.price     = md_trade->sell_price_4();
        m_md_current->sell_4.quantity  = md_trade->sell_quantity_4();
        m_md_current->sell_3.price     = md_trade->sell_price_3();
        m_md_current->sell_3.quantity  = md_trade->sell_quantity_3();
        m_md_current->sell_2.price     = md_trade->sell_price_2();
        m_md_current->sell_2.quantity  = md_trade->sell_quantity_2();
        m_md_current->sell_1.price     = md_trade->sell_price_1();
        m_md_current->sell_1.quantity  = md_trade->sell_quantity_1();
        m_md_current->buy_1.price      = md_trade->buy_price_1();
        m_md_current->buy_1.quantity   = md_trade->buy_quantity_1();
        m_md_current->buy_2.price      = md_trade->buy_price_2();
        m_md_current->buy_2.quantity   = md_trade->buy_quantity_2();
        m_md_current->buy_3.price      = md_trade->buy_price_3();
        m_md_current->buy_3.quantity   = md_trade->buy_quantity_3();
        m_md_current->buy_4.price      = md_trade->buy_price_4();
        m_md_current->buy_4.quantity   = md_trade->buy_quantity_4();
        m_md_current->buy_5.price      = md_trade->buy_price_5();
        m_md_current->buy_5.quantity   = md_trade->buy_quantity_5();
        m_md_current->buy_6.price      = md_trade->buy_price_6();
        m_md_current->buy_6.quantity   = md_trade->buy_quantity_6();
        m_md_current->buy_7.price      = md_trade->buy_price_7();
        m_md_current->buy_7.quantity   = md_trade->buy_quantity_7();
        m_md_current->buy_8.price      = md_trade->buy_price_8();
        m_md_current->buy_8.quantity   = md_trade->buy_quantity_8();
        m_md_current->buy_9.price      = md_trade->buy_price_9();
        m_md_current->buy_9.quantity   = md_trade->buy_quantity_9();
        m_md_current->buy_10.price     = md_trade->buy_price_10();
        m_md_current->buy_10.quantity  = md_trade->buy_quantity_10();

        m_md_current++;
        m_shm_mate_info->market_data_count++;
        M_A {m_shm_mate_info->last_update_time} = utilities::Now<std::string>()();
    }

    m_outside->md_trade_generated(md_trade);
}
