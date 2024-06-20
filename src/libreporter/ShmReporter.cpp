#include "libreporter/ShmReporter.h"
#include "utilities/MakeAssignable.hpp"
#include "utilities/TimeHelper.hpp"

#define REMOVE_DATE(time) (time % 1000000000)

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
    m_tick_region       = std::make_shared<boost::interprocess::mapped_region>(m_shm, boost::interprocess::read_write, 0 * allocated_shm_size / 2, allocated_shm_size / 2);
    m_l2_tick_region    = std::make_shared<boost::interprocess::mapped_region>(m_shm, boost::interprocess::read_write, 1 * allocated_shm_size / 2, allocated_shm_size / 2);

    m_tick_mate_info    = static_cast<SMTickMateInfo*>(m_tick_region->get_address());
    m_l2_tick_mate_info = static_cast<SML2TickMateInfo*>(m_l2_tick_region->get_address());

    /// Set memory areas.
    memset(m_tick_region->get_address(), 0, allocated_shm_size / 2);
    memset(m_l2_tick_region->get_address(), 0, allocated_shm_size / 2);

    /// Assign pointers with start address.
    m_tick_start      = reinterpret_cast<Tick*>(m_tick_mate_info + 1);
    m_tick_current    = m_tick_start;
    m_l2_tick_start   = reinterpret_cast<L2Tick*>(m_l2_tick_mate_info + 1);
    m_l2_tick_current = m_l2_tick_start;
}

void trade::reporter::ShmReporter::exchange_tick_arrived(const std::shared_ptr<types::ExchangeTick> exchange_tick)
{
    do_exchange_tick_report(exchange_tick);
    m_outside->exchange_tick_arrived(exchange_tick);
}

void trade::reporter::ShmReporter::exchange_l2_tick_arrived(const std::shared_ptr<types::L2Tick> l2_tick)
{
    do_l2_tick_report(l2_tick, ShmUnionType::l2_tick_from_exchange);
    m_outside->exchange_l2_tick_arrived(l2_tick);
}

void trade::reporter::ShmReporter::l2_tick_generated(const std::shared_ptr<types::L2Tick> l2_tick)
{
    do_l2_tick_report(l2_tick, ShmUnionType::self_generated_l2_tick);
    m_outside->l2_tick_generated(l2_tick);
}

void trade::reporter::ShmReporter::do_exchange_tick_report(const std::shared_ptr<types::ExchangeTick>& exchange_tick)
{
    /// Check if shared memory is full.
    if (m_tick_current - m_tick_start > m_tick_region->get_size() / sizeof(Tick)) {
        logger->error("Shared memory of tick is full");
        m_outside->exchange_tick_arrived(exchange_tick);
        return;
    }

    /// Writing scope.
    {
        boost::interprocess::scoped_lock lock(m_named_mutex);

        m_tick_current->shm_union_type    = ShmUnionType::tick_from_exchange;

        M_A {m_tick_current->symbol}      = exchange_tick->symbol();
        m_tick_current->exec_price        = exchange_tick->exec_price();
        m_tick_current->exec_quantity     = exchange_tick->exec_quantity();
        m_tick_current->bid_unique_id     = exchange_tick->bid_unique_id();
        m_tick_current->ask_unique_id     = exchange_tick->ask_unique_id();

        m_tick_current->exhange_time      = REMOVE_DATE(utilities::ToTime<int64_t>()(exchange_tick->exchange_time()));
        m_tick_current->local_system_time = REMOVE_DATE(utilities::Now<int64_t>()());

        m_tick_current++;
        m_tick_mate_info->tick_count++;
        m_tick_mate_info->last_update_time = REMOVE_DATE(utilities::Now<int64_t>()());
    }
}

void trade::reporter::ShmReporter::do_l2_tick_report(const std::shared_ptr<types::L2Tick>& l2_tick, const ShmUnionType shm_union_type)
{
    /// Check if shared memory is full.
    if (m_l2_tick_current - m_l2_tick_start > m_l2_tick_region->get_size() / sizeof(L2Tick)) {
        logger->error("Shared memory of l2 tick is full");
        m_outside->l2_tick_generated(l2_tick);
        return;
    }

    /// Writing scope.
    {
        boost::interprocess::scoped_lock lock(m_named_mutex);

        m_l2_tick_current->shm_union_type    = shm_union_type;

        M_A {m_l2_tick_current->symbol}      = l2_tick->symbol();
        m_l2_tick_current->price             = l2_tick->price();
        m_l2_tick_current->quantity          = static_cast<int32_t>(l2_tick->quantity());

        m_l2_tick_current->sell_10.price     = l2_tick->sell_price_10();
        m_l2_tick_current->sell_10.quantity  = l2_tick->sell_quantity_10();
        m_l2_tick_current->sell_9.price      = l2_tick->sell_price_9();
        m_l2_tick_current->sell_9.quantity   = l2_tick->sell_quantity_9();
        m_l2_tick_current->sell_8.price      = l2_tick->sell_price_8();
        m_l2_tick_current->sell_8.quantity   = l2_tick->sell_quantity_8();
        m_l2_tick_current->sell_7.price      = l2_tick->sell_price_7();
        m_l2_tick_current->sell_7.quantity   = l2_tick->sell_quantity_7();
        m_l2_tick_current->sell_6.price      = l2_tick->sell_price_6();
        m_l2_tick_current->sell_6.quantity   = l2_tick->sell_quantity_6();
        m_l2_tick_current->sell_5.price      = l2_tick->sell_price_5();
        m_l2_tick_current->sell_5.quantity   = l2_tick->sell_quantity_5();
        m_l2_tick_current->sell_4.price      = l2_tick->sell_price_4();
        m_l2_tick_current->sell_4.quantity   = l2_tick->sell_quantity_4();
        m_l2_tick_current->sell_3.price      = l2_tick->sell_price_3();
        m_l2_tick_current->sell_3.quantity   = l2_tick->sell_quantity_3();
        m_l2_tick_current->sell_2.price      = l2_tick->sell_price_2();
        m_l2_tick_current->sell_2.quantity   = l2_tick->sell_quantity_2();
        m_l2_tick_current->sell_1.price      = l2_tick->sell_price_1();
        m_l2_tick_current->sell_1.quantity   = l2_tick->sell_quantity_1();
        m_l2_tick_current->buy_1.price       = l2_tick->buy_price_1();
        m_l2_tick_current->buy_1.quantity    = l2_tick->buy_quantity_1();
        m_l2_tick_current->buy_2.price       = l2_tick->buy_price_2();
        m_l2_tick_current->buy_2.quantity    = l2_tick->buy_quantity_2();
        m_l2_tick_current->buy_3.price       = l2_tick->buy_price_3();
        m_l2_tick_current->buy_3.quantity    = l2_tick->buy_quantity_3();
        m_l2_tick_current->buy_4.price       = l2_tick->buy_price_4();
        m_l2_tick_current->buy_4.quantity    = l2_tick->buy_quantity_4();
        m_l2_tick_current->buy_5.price       = l2_tick->buy_price_5();
        m_l2_tick_current->buy_5.quantity    = l2_tick->buy_quantity_5();
        m_l2_tick_current->buy_6.price       = l2_tick->buy_price_6();
        m_l2_tick_current->buy_6.quantity    = l2_tick->buy_quantity_6();
        m_l2_tick_current->buy_7.price       = l2_tick->buy_price_7();
        m_l2_tick_current->buy_7.quantity    = l2_tick->buy_quantity_7();
        m_l2_tick_current->buy_8.price       = l2_tick->buy_price_8();
        m_l2_tick_current->buy_8.quantity    = l2_tick->buy_quantity_8();
        m_l2_tick_current->buy_9.price       = l2_tick->buy_price_9();
        m_l2_tick_current->buy_9.quantity    = l2_tick->buy_quantity_9();
        m_l2_tick_current->buy_10.price      = l2_tick->buy_price_10();
        m_l2_tick_current->buy_10.quantity   = l2_tick->buy_quantity_10();

        m_l2_tick_current->exhange_time      = REMOVE_DATE(utilities::ToTime<int64_t>()(l2_tick->exchange_time()));
        m_l2_tick_current->local_system_time = REMOVE_DATE(utilities::Now<int64_t>()());

        m_l2_tick_current++;
        m_l2_tick_mate_info->l2_tick_count++;
        m_l2_tick_mate_info->last_update_time = REMOVE_DATE(utilities::Now<int64_t>()());
    }
}
