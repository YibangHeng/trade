#include "libreporter/ShmReporter.h"
#include "utilities/MakeAssignable.hpp"
#include "utilities/TimeHelper.hpp"

#define REMOVE_DATE(time) (time % 1000000000)

trade::reporter::ShmReporter::ShmReporter(
    const std::string& shm_name,
    const std::string& shm_mutex_name,
    const int shm_size,
    const std::shared_ptr<IReporter>& outside
)
    : AppBase("ShmReporter"),
      NopReporter(outside),
      m_shm(boost::interprocess::open_or_create, shm_name.c_str(), boost::interprocess::read_write),
      m_named_mutex(boost::interprocess::open_or_create, shm_mutex_name.c_str()),
      m_outside(outside)
{
    m_shm.truncate(shm_size * GB);

    boost::interprocess::offset_t allocated_shm_size;
    m_shm.get_size(allocated_shm_size);

    logger->info("Created/Opened shared memory {} with size {}GB ({} bytes)", m_shm.get_name(), allocated_shm_size / GB, allocated_shm_size);

    /// Map areas of shared memory with same size.
    m_order_tick_region    = std::make_shared<boost::interprocess::mapped_region>(m_shm, boost::interprocess::read_write, 0 * allocated_shm_size / 3, allocated_shm_size / 3);
    m_trade_tick_region    = std::make_shared<boost::interprocess::mapped_region>(m_shm, boost::interprocess::read_write, 1 * allocated_shm_size / 3, allocated_shm_size / 3);
    m_l2_tick_region       = std::make_shared<boost::interprocess::mapped_region>(m_shm, boost::interprocess::read_write, 2 * allocated_shm_size / 3, allocated_shm_size / 3);

    m_order_tick_mate_info = static_cast<SMTickMateInfo*>(m_order_tick_region->get_address());
    m_trade_tick_mate_info = static_cast<SMTickMateInfo*>(m_trade_tick_region->get_address());
    m_l2_tick_mate_info    = static_cast<SML2TickMateInfo*>(m_l2_tick_region->get_address());

    /// Set memory areas.
    memset(m_order_tick_region->get_address(), 0, allocated_shm_size / 3);
    memset(m_trade_tick_region->get_address(), 0, allocated_shm_size / 3);
    memset(m_l2_tick_region->get_address(), 0, allocated_shm_size / 3);

    /// Assign pointers with start address.
    m_order_tick_start   = reinterpret_cast<OrderTick*>(m_order_tick_mate_info + 1);
    m_order_tick_current = m_order_tick_start;
    m_trade_tick_start   = reinterpret_cast<TradeTick*>(m_trade_tick_mate_info + 1);
    m_trade_tick_current = m_trade_tick_start;
    m_l2_tick_start      = reinterpret_cast<L2Tick*>(m_l2_tick_mate_info + 1);
    m_l2_tick_current    = m_l2_tick_start;
}

void trade::reporter::ShmReporter::exchange_order_tick_arrived(const std::shared_ptr<types::OrderTick> order_tick)
{
    do_exchange_order_tick_report(order_tick);
    m_outside->exchange_order_tick_arrived(order_tick);
}

void trade::reporter::ShmReporter::exchange_trade_tick_arrived(std::shared_ptr<types::TradeTick> trade_tick)
{
    do_exchange_trade_tick_report(trade_tick);
    m_outside->exchange_trade_tick_arrived(trade_tick);
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

void trade::reporter::ShmReporter::do_exchange_order_tick_report(const std::shared_ptr<types::OrderTick>& order_tick)
{
    /// Check if shared memory is full.
    if (m_order_tick_current - m_order_tick_start > m_order_tick_region->get_size() / sizeof(OrderTick)) {
        logger->warn("Shared memory of tick is full");
        m_order_tick_current = m_order_tick_start;
    }

    /// Writing scope.
    {
        boost::interprocess::scoped_lock lock(m_named_mutex);

        m_order_tick_current->shm_union_type = ShmUnionType::order_tick_from_exchange;

        m_order_tick_current->unique_id      = order_tick->unique_id();
        /// TODO: Use convertor here.
        if (order_tick->order_type() == types::OrderType::cancel)
            m_order_tick_current->order_type = static_cast<OrderType>(order_tick->order_type());
        else
            m_order_tick_current->order_type = static_cast<OrderType>(order_tick->side());
        M_A {m_order_tick_current->symbol}      = order_tick->symbol();
        m_order_tick_current->price_1000x       = order_tick->price_1000x();
        m_order_tick_current->quantity          = order_tick->quantity();

        m_order_tick_current->exhange_time      = REMOVE_DATE(order_tick->exchange_time());
        m_order_tick_current->local_system_time = REMOVE_DATE(utilities::Now<int64_t>()());

        m_order_tick_current++;
        m_order_tick_mate_info->tick_count++;
        m_order_tick_mate_info->last_update_time = REMOVE_DATE(utilities::Now<int64_t>()());
    }
}

void trade::reporter::ShmReporter::do_exchange_trade_tick_report(const std::shared_ptr<types::TradeTick>& trade_tick)
{
    /// Check if shared memory is full.
    if (m_trade_tick_current - m_trade_tick_start > m_trade_tick_region->get_size() / sizeof(TradeTick)) {
        logger->warn("Shared memory of tick is full");
        m_trade_tick_current = m_trade_tick_start;
    }

    /// Writing scope.
    {
        boost::interprocess::scoped_lock lock(m_named_mutex);

        m_trade_tick_current->shm_union_type    = ShmUnionType::trade_tick_from_exchange;

        m_trade_tick_current->ask_unique_id     = trade_tick->ask_unique_id();
        m_trade_tick_current->bid_unique_id     = trade_tick->bid_unique_id();
        M_A {m_trade_tick_current->symbol}      = trade_tick->symbol();
        m_trade_tick_current->exec_price_1000x  = trade_tick->exec_price_1000x();
        m_trade_tick_current->exec_quantity     = trade_tick->exec_quantity();

        m_trade_tick_current->exchange_time     = REMOVE_DATE(trade_tick->exchange_time());
        m_trade_tick_current->local_system_time = REMOVE_DATE(utilities::Now<int64_t>()());

        m_trade_tick_current++;
        m_trade_tick_mate_info->tick_count++;
        m_trade_tick_mate_info->last_update_time = REMOVE_DATE(utilities::Now<int64_t>()());
    }
}

void trade::reporter::ShmReporter::do_l2_tick_report(const std::shared_ptr<types::L2Tick>& l2_tick, const ShmUnionType shm_union_type)
{
    /// Check if shared memory is full.
    if (m_l2_tick_current - m_l2_tick_start > m_l2_tick_region->get_size() / sizeof(L2Tick)) {
        logger->warn("Shared memory of tick is full");
        m_l2_tick_current = m_l2_tick_start;
    }

    /// Writing scope.
    {
        boost::interprocess::scoped_lock lock(m_named_mutex);

        m_l2_tick_current->shm_union_type     = shm_union_type;

        M_A {m_l2_tick_current->symbol}       = l2_tick->symbol();
        m_l2_tick_current->price_1000x        = l2_tick->price();
        m_l2_tick_current->quantity           = static_cast<int32_t>(l2_tick->quantity());

        m_l2_tick_current->sell_5.price_1000x = l2_tick->sell_price_5();
        m_l2_tick_current->sell_5.quantity    = l2_tick->sell_quantity_5();
        m_l2_tick_current->sell_4.price_1000x = l2_tick->sell_price_4();
        m_l2_tick_current->sell_4.quantity    = l2_tick->sell_quantity_4();
        m_l2_tick_current->sell_3.price_1000x = l2_tick->sell_price_3();
        m_l2_tick_current->sell_3.quantity    = l2_tick->sell_quantity_3();
        m_l2_tick_current->sell_2.price_1000x = l2_tick->sell_price_2();
        m_l2_tick_current->sell_2.quantity    = l2_tick->sell_quantity_2();
        m_l2_tick_current->sell_1.price_1000x = l2_tick->sell_price_1();
        m_l2_tick_current->sell_1.quantity    = l2_tick->sell_quantity_1();
        m_l2_tick_current->buy_1.price_1000x  = l2_tick->buy_price_1();
        m_l2_tick_current->buy_1.quantity     = l2_tick->buy_quantity_1();
        m_l2_tick_current->buy_2.price_1000x  = l2_tick->buy_price_2();
        m_l2_tick_current->buy_2.quantity     = l2_tick->buy_quantity_2();
        m_l2_tick_current->buy_3.price_1000x  = l2_tick->buy_price_3();
        m_l2_tick_current->buy_3.quantity     = l2_tick->buy_quantity_3();
        m_l2_tick_current->buy_4.price_1000x  = l2_tick->buy_price_4();
        m_l2_tick_current->buy_4.quantity     = l2_tick->buy_quantity_4();
        m_l2_tick_current->buy_5.price_1000x  = l2_tick->buy_price_5();
        m_l2_tick_current->buy_5.quantity     = l2_tick->buy_quantity_5();

        m_l2_tick_current->exchange_time      = REMOVE_DATE(l2_tick->exchange_time());
        m_l2_tick_current->local_system_time  = REMOVE_DATE(utilities::Now<int64_t>()());

        m_l2_tick_current++;
        m_l2_tick_mate_info->l2_tick_count++;
        m_l2_tick_mate_info->last_update_time = REMOVE_DATE(utilities::Now<int64_t>()());
    }
}
