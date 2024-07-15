#pragma once

#include <boost/interprocess/mapped_region.hpp>
#include <boost/interprocess/shared_memory_object.hpp>
#include <boost/interprocess/sync/named_upgradable_mutex.hpp>
#include <limits>

#include "AppBase.hpp"
#include "IReporter.hpp"
#include "NopReporter.hpp"

namespace trade::reporter
{

/// For using memcpy() in shared memory.
#pragma pack(push, 1)

/// Ensure that double fulfills the requirements of the IEC 559 (IEEE 754)
/// standard.
static_assert(std::numeric_limits<double>::is_iec559);

/// TODO: Use high precision library such as Boost.Multiprecision or GMP to deal
/// with double precision.

#define RESERVED(bytes) \
private:                \
    u_char m_reserved[bytes] = {};

using SymbolType = char[16];

/// @types::OrderType and @types::SideType.
enum class OrderType
{
    invalid_side = 0,

    buy          = 1000, /// 买
    sell         = 1001, /// 卖

    cancel       = 2000, /// 撤单
};

static_assert(static_cast<int>(OrderType::buy) == static_cast<int>(types::SideType::buy));
static_assert(static_cast<int>(OrderType::sell) == static_cast<int>(types::SideType::sell));
static_assert(static_cast<int>(OrderType::cancel) == static_cast<int>(types::OrderType::cancel));
static_assert(sizeof(OrderType) == 4, "OrderType should be 4 bytes");

struct PUBLIC_API SMTickMateInfo {
    size_t tick_count        = 0;
    int64_t last_update_time = {}; /// Time in ISO 8601 format.
    RESERVED(496)
};

static_assert(sizeof(SMTickMateInfo) == 512, "MateInfo for tick should be 512 bytes");

struct PUBLIC_API SML2TickMateInfo {
    size_t l2_tick_count     = 0;
    int64_t last_update_time = {}; /// Time in ISO 8601 format.
    RESERVED(496)
};

static_assert(sizeof(SML2TickMateInfo) == 512, "MateInfo for l2 tick should be 512 bytes");

struct PriceQuantityPair {
    int64_t price_1000x = 0;
    int64_t quantity    = 0;
};

static_assert(sizeof(PriceQuantityPair) == 16, "PriceQuantityPair should be 16 bytes");

enum class ShmUnionType
{
    invalid_shm_union        = 0,

    order_tick_from_exchange = 1, /// 交易所逐笔委托
    trade_tick_from_exchange = 2, /// 交易所逐笔成交
    l2_tick_from_exchange    = 3, /// 交易所行情切片
    self_generated_l2_tick   = 4, /// 撮合行情
};

static_assert(sizeof(ShmUnionType) == 4, "ShmUnionType should be 4 bytes");

/// @types::OrderTick.
struct PUBLIC_API OrderTick {
    ShmUnionType shm_union_type;

    int64_t unique_id    = 0;
    OrderType order_type = OrderType::invalid_side;
    SymbolType symbol    = {};
    /// Side info stored in @order_type.
    int64_t price_1000x       = 0;
    int64_t quantity          = 0;

    int64_t exhange_time      = 0;
    int64_t local_system_time = 0;

    RESERVED(448)
};

static_assert(sizeof(OrderTick) == 512, "Tick should be 512 bytes");

/// @types::TradeTick.
struct PUBLIC_API TradeTick {
    ShmUnionType shm_union_type;

    int64_t ask_unique_id     = 0;
    int64_t bid_unique_id     = 0;
    SymbolType symbol         = {};
    int64_t exec_price_1000x  = 0;
    int64_t exec_quantity     = 0;

    int64_t exchange_time     = 0;
    int64_t local_system_time = 0;

    RESERVED(444)
};

static_assert(sizeof(TradeTick) == 512, "Tick should be 512 bytes");

struct PUBLIC_API L2Tick {
    ShmUnionType shm_union_type;

    SymbolType symbol   = {};
    int64_t price_1000x = 0;
    int64_t quantity    = 0;

    PriceQuantityPair sell_10;
    PriceQuantityPair sell_9;
    PriceQuantityPair sell_8;
    PriceQuantityPair sell_7;
    PriceQuantityPair sell_6;
    PriceQuantityPair sell_5;
    PriceQuantityPair sell_4;
    PriceQuantityPair sell_3;
    PriceQuantityPair sell_2;
    PriceQuantityPair sell_1;
    PriceQuantityPair buy_1;
    PriceQuantityPair buy_2;
    PriceQuantityPair buy_3;
    PriceQuantityPair buy_4;
    PriceQuantityPair buy_5;
    PriceQuantityPair buy_6;
    PriceQuantityPair buy_7;
    PriceQuantityPair buy_8;
    PriceQuantityPair buy_9;
    PriceQuantityPair buy_10;

    int64_t exchange_time     = 0;
    int64_t local_system_time = 0;

    RESERVED(140)
};

static_assert(sizeof(L2Tick) == 512, "L2Tick should be 512 bytes");

union PUBLIC_API ShmUnion
{
    OrderTick order_tick;
    TradeTick trade_tick;
    L2Tick market_data;
};

static_assert(sizeof(ShmUnion) == 512, "ShmUnion should be 512 bytes");

#pragma pack(pop)

/// ShmReporter writes market data to shared memory.
class PUBLIC_API ShmReporter final: private AppBase<>, public NopReporter
{
public:
    explicit ShmReporter(
        const std::string& shm_name        = "trade_data",
        const std::string& shm_mutex_name  = "trade_data_mutex",
        int shm_size                       = 1,
        std::shared_ptr<IReporter> outside = std::make_shared<NopReporter>()
    );
    ~ShmReporter() override = default;

    /// Market data.
public:
    void exchange_order_tick_arrived(std::shared_ptr<types::OrderTick> order_tick) override;
    void exchange_trade_tick_arrived(std::shared_ptr<types::TradeTick> trade_tick) override;
    void exchange_l2_tick_arrived(std::shared_ptr<types::L2Tick> l2_tick) override;
    void l2_tick_generated(std::shared_ptr<types::L2Tick> l2_tick) override;

private:
    void do_exchange_order_tick_report(const std::shared_ptr<types::OrderTick>& order_tick);
    void do_exchange_trade_tick_report(const std::shared_ptr<types::TradeTick>& trade_tick);
    void do_l2_tick_report(const std::shared_ptr<types::L2Tick>& l2_tick, ShmUnionType shm_union_type);

private:
    /// Return the start memory address of trade area.
    boost::interprocess::offset_t trade_offset();

private:
    boost::interprocess::shared_memory_object m_shm;
    std::shared_ptr<boost::interprocess::mapped_region> m_order_tick_region;
    std::shared_ptr<boost::interprocess::mapped_region> m_trade_tick_region;
    std::shared_ptr<boost::interprocess::mapped_region> m_l2_tick_region;
    boost::interprocess::named_upgradable_mutex m_named_mutex;
    SMTickMateInfo* m_order_tick_mate_info;
    SMTickMateInfo* m_trade_tick_mate_info;
    SML2TickMateInfo* m_l2_tick_mate_info;
    /// Store the start/current memory address of tick area.
    OrderTick* m_order_tick_start;
    OrderTick* m_order_tick_current;
    TradeTick* m_trade_tick_start;
    TradeTick* m_trade_tick_current;
    L2Tick* m_l2_tick_start;
    L2Tick* m_l2_tick_current;

private:
    static constexpr boost::interprocess::offset_t GB = 1024 * 1024 * 1024;

private:
    std::shared_ptr<IReporter> m_outside;
};

} // namespace trade::reporter
