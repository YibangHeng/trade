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
      m_shm(boost::interprocess::open_or_create, shm_name.c_str(), boost::interprocess::read_write),
      m_named_mutex(boost::interprocess::open_or_create, shm_mutex_name.c_str()),
      m_outside(std::move(outside))
{
    m_shm.truncate(shm_size * GB);

    boost::interprocess::offset_t allocated_shm_size;
    m_shm.get_size(allocated_shm_size);

    logger->info("Created/Opened shared memory {} with size {}GB ({} bytes)", m_shm.get_name(), allocated_shm_size / GB, allocated_shm_size);

    /// Map areas of shared memory with same size.
    m_tick_region           = std::make_shared<boost::interprocess::mapped_region>(m_shm, boost::interprocess::read_write, 0 * allocated_shm_size / 2, allocated_shm_size / 2);
    m_market_data_region    = std::make_shared<boost::interprocess::mapped_region>(m_shm, boost::interprocess::read_write, 1 * allocated_shm_size / 2, allocated_shm_size / 2);

    m_market_data_mate_info = static_cast<SMMarketDataMateInfo*>(m_market_data_region->get_address());

    /// Set memory areas.
    memset(m_tick_region->get_address(), 0, allocated_shm_size / 2);
    memset(m_market_data_region->get_address(), 0, allocated_shm_size / 2);

    /// Assign pointers with start address.
    m_market_data_start   = reinterpret_cast<L2Tick*>(m_market_data_mate_info + 1);
    m_market_data_current = m_market_data_start;

    logger->info("Divided shared memory areas to trade({})", m_market_data_region->get_address());
}

void trade::reporter::ShmReporter::exchange_l2_tick_arrived(std::shared_ptr<types::L2Tick> l2_tick)
{
    do_l2_tick_report(l2_tick, ShmUnionType::tick_from_exchange);
    m_outside->exchange_l2_tick_arrived(l2_tick);
}

void trade::reporter::ShmReporter::l2_tick_generated(const std::shared_ptr<types::L2Tick> l2_tick)
{
    do_l2_tick_report(l2_tick, ShmUnionType::self_generated_market_data);
    m_outside->l2_tick_generated(l2_tick);
}

void trade::reporter::ShmReporter::do_l2_tick_report(const std::shared_ptr<types::L2Tick>& l2_tick, const ShmUnionType shm_union_type)
{
    /// Check if shared memory is full.
    if (m_market_data_current - m_market_data_start > m_market_data_region->get_size() / sizeof(L2Tick)) {
        logger->error("Shared memory of market data is full");
        m_outside->l2_tick_generated(l2_tick);
        return;
    }

    /// Writing scope.
    {
        boost::interprocess::scoped_lock lock(m_named_mutex);

        m_market_data_current->shm_union_type   = shm_union_type;

        M_A {m_market_data_current->symbol}     = l2_tick->symbol();
        m_market_data_current->price            = l2_tick->price();
        m_market_data_current->quantity         = static_cast<int32_t>(l2_tick->quantity());

        m_market_data_current->sell_10.price    = l2_tick->sell_price_10();
        m_market_data_current->sell_10.quantity = l2_tick->sell_quantity_10();
        m_market_data_current->sell_9.price     = l2_tick->sell_price_9();
        m_market_data_current->sell_9.quantity  = l2_tick->sell_quantity_9();
        m_market_data_current->sell_8.price     = l2_tick->sell_price_8();
        m_market_data_current->sell_8.quantity  = l2_tick->sell_quantity_8();
        m_market_data_current->sell_7.price     = l2_tick->sell_price_7();
        m_market_data_current->sell_7.quantity  = l2_tick->sell_quantity_7();
        m_market_data_current->sell_6.price     = l2_tick->sell_price_6();
        m_market_data_current->sell_6.quantity  = l2_tick->sell_quantity_6();
        m_market_data_current->sell_5.price     = l2_tick->sell_price_5();
        m_market_data_current->sell_5.quantity  = l2_tick->sell_quantity_5();
        m_market_data_current->sell_4.price     = l2_tick->sell_price_4();
        m_market_data_current->sell_4.quantity  = l2_tick->sell_quantity_4();
        m_market_data_current->sell_3.price     = l2_tick->sell_price_3();
        m_market_data_current->sell_3.quantity  = l2_tick->sell_quantity_3();
        m_market_data_current->sell_2.price     = l2_tick->sell_price_2();
        m_market_data_current->sell_2.quantity  = l2_tick->sell_quantity_2();
        m_market_data_current->sell_1.price     = l2_tick->sell_price_1();
        m_market_data_current->sell_1.quantity  = l2_tick->sell_quantity_1();
        m_market_data_current->buy_1.price      = l2_tick->buy_price_1();
        m_market_data_current->buy_1.quantity   = l2_tick->buy_quantity_1();
        m_market_data_current->buy_2.price      = l2_tick->buy_price_2();
        m_market_data_current->buy_2.quantity   = l2_tick->buy_quantity_2();
        m_market_data_current->buy_3.price      = l2_tick->buy_price_3();
        m_market_data_current->buy_3.quantity   = l2_tick->buy_quantity_3();
        m_market_data_current->buy_4.price      = l2_tick->buy_price_4();
        m_market_data_current->buy_4.quantity   = l2_tick->buy_quantity_4();
        m_market_data_current->buy_5.price      = l2_tick->buy_price_5();
        m_market_data_current->buy_5.quantity   = l2_tick->buy_quantity_5();
        m_market_data_current->buy_6.price      = l2_tick->buy_price_6();
        m_market_data_current->buy_6.quantity   = l2_tick->buy_quantity_6();
        m_market_data_current->buy_7.price      = l2_tick->buy_price_7();
        m_market_data_current->buy_7.quantity   = l2_tick->buy_quantity_7();
        m_market_data_current->buy_8.price      = l2_tick->buy_price_8();
        m_market_data_current->buy_8.quantity   = l2_tick->buy_quantity_8();
        m_market_data_current->buy_9.price      = l2_tick->buy_price_9();
        m_market_data_current->buy_9.quantity   = l2_tick->buy_quantity_9();
        m_market_data_current->buy_10.price     = l2_tick->buy_price_10();
        m_market_data_current->buy_10.quantity  = l2_tick->buy_quantity_10();

        m_market_data_current++;
        m_market_data_mate_info->market_data_count++;
        M_A {m_market_data_mate_info->last_update_time} = utilities::Now<std::string>()();
    }
}
