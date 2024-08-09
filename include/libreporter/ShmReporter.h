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

struct TD_PUBLIC_API SMTickMateInfo {
    size_t tick_count        = 0;
    int64_t last_update_time = 0; /// Time in ISO 8601 format.
    RESERVED(240)
};

static_assert(sizeof(SMTickMateInfo) == 256, "MateInfo for tick should be 256 bytes");

struct TD_PUBLIC_API SMExchangeL2SnapMateInfo {
    size_t exchange_l2_snap_count = 0;
    int64_t last_update_time      = 0; /// Time in ISO 8601 format.
    RESERVED(240)
};

static_assert(sizeof(SMExchangeL2SnapMateInfo) == 256, "MateInfo for exchange l2 sanp should be 256 bytes");

struct TD_PUBLIC_API SMGeneratedL2TickMateInfo {
    size_t generated_l2_tick_count = 0;
    int64_t last_update_time       = 0; /// Time in ISO 8601 format.
    RESERVED(240)
};

static_assert(sizeof(SMGeneratedL2TickMateInfo) == 256, "MateInfo for generated l2 tick should be 256 bytes");

struct PriceQuantityPair {
    uint32_t price_1000x = 0;
    uint32_t quantity    = 0;
};

static_assert(sizeof(PriceQuantityPair) == 8, "PriceQuantityPair should be 16 bytes");

enum class ShmUnionType
{
    invalid_shm_union        = 0,

    order_tick_from_exchange = 1, /// 交易所逐笔委托
    trade_tick_from_exchange = 2, /// 交易所逐笔成交
    l2_snap_from_exchange    = 3, /// 交易所行情切片
    generated_l2_tick        = 4, /// 逐笔撮合行情
};

static_assert(sizeof(ShmUnionType) == 4, "ShmUnionType should be 4 bytes");

/// @types::OrderTick.
struct TD_PUBLIC_API OrderTick {
    ShmUnionType shm_union_type;

    int64_t unique_id    = 0;
    OrderType order_type = OrderType::invalid_side;
    SymbolType symbol    = {};
    /// Side info stored in @order_type.
    int64_t price_1000x       = 0;
    int64_t quantity          = 0;

    int64_t exhange_time      = 0;
    int64_t local_system_time = 0;

    RESERVED(192)
};

static_assert(sizeof(OrderTick) == 256, "Tick should be 256 bytes");

/// @types::TradeTick.
struct TD_PUBLIC_API TradeTick {
    ShmUnionType shm_union_type;

    int64_t ask_unique_id     = 0;
    int64_t bid_unique_id     = 0;
    SymbolType symbol         = {};
    int64_t exec_price_1000x  = 0;
    int64_t exec_quantity     = 0;

    int64_t exchange_time     = 0;
    int64_t local_system_time = 0;

    RESERVED(188)
};

static_assert(sizeof(TradeTick) == 256, "Tick should be 256 bytes");

struct TD_PUBLIC_API ExchangeL2Snap {
    ShmUnionType shm_union_type;

    SymbolType symbol               = {};
    int64_t price_1000x             = 0;

    int64_t pre_settlement_1000x    = 0;
    int64_t pre_close_price_1000x   = 0;
    int64_t open_price_1000x        = 0;
    int64_t highest_price_1000x     = 0;
    int64_t lowest_price_1000x      = 0;
    int64_t close_price_1000x       = 0;
    int64_t settlement_price_1000x  = 0;
    int64_t upper_limit_price_1000x = 0;
    int64_t lower_limit_price_1000x = 0;

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

    int64_t exchange_time     = 0;
    int64_t local_system_time = 0;

    RESERVED(60)
};

static_assert(sizeof(ExchangeL2Snap) == 256, "L2Tick should be 256 bytes");

struct TD_PUBLIC_API GeneratedL2Tick {
    ShmUnionType shm_union_type;

    SymbolType symbol     = {};
    int64_t price_1000x   = 0;
    int64_t quantity      = 0;
    int64_t ask_unique_id = 0;
    int64_t bid_unique_id = 0;

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

    int64_t exchange_time     = 0;
    int64_t local_system_time = 0;

    RESERVED(108)
};

static_assert(sizeof(GeneratedL2Tick) == 256, "L2Tick should be 256 bytes");

union TD_PUBLIC_API ShmUnion
{
    OrderTick order_tick;
    TradeTick trade_tick;
    GeneratedL2Tick market_data;
};

static_assert(sizeof(ShmUnion) == 256, "ShmUnion should be 256 bytes");

#pragma pack(pop)

/// ShmReporter writes market data to shared memory.
class TD_PUBLIC_API ShmReporter final: private AppBase<>, public NopReporter
{
public:
    explicit ShmReporter(
        const std::string& shm_name               = "trade_data",
        const std::string& shm_mutex_name         = "trade_data_mutex",
        int shm_size                              = 1,
        const std::shared_ptr<IReporter>& outside = std::make_shared<NopReporter>()
    );
    ~ShmReporter() override = default;

    /// Market data.
public:
    void exchange_order_tick_arrived(std::shared_ptr<types::OrderTick> order_tick) override;
    void exchange_trade_tick_arrived(std::shared_ptr<types::TradeTick> trade_tick) override;
    void exchange_l2_snap_arrived(std::shared_ptr<types::ExchangeL2Snap> exchange_l2_snap) override;
    void l2_tick_generated(std::shared_ptr<types::GeneratedL2Tick> generated_l2_tick) override;

private:
    void do_exchange_order_tick_report(const std::shared_ptr<types::OrderTick>& order_tick);
    void do_exchange_trade_tick_report(const std::shared_ptr<types::TradeTick>& trade_tick);
    void do_exchange_l2_snap_report(const std::shared_ptr<types::ExchangeL2Snap>& exchange_l2_snap);
    void do_generated_l2_tick_report(const std::shared_ptr<types::GeneratedL2Tick>& generated_l2_tick);

private:
    /// Return the start memory address of trade area.
    boost::interprocess::offset_t trade_offset();

private:
    boost::interprocess::shared_memory_object m_shm;
    boost::interprocess::named_upgradable_mutex m_named_mutex;
    std::shared_ptr<boost::interprocess::mapped_region> m_order_tick_region;
    std::shared_ptr<boost::interprocess::mapped_region> m_trade_tick_region;
    std::shared_ptr<boost::interprocess::mapped_region> m_exchange_l2_snap_region;
    std::shared_ptr<boost::interprocess::mapped_region> m_generated_l2_tick_region;
    SMTickMateInfo* m_order_tick_mate_info;
    SMTickMateInfo* m_trade_tick_mate_info;
    SMExchangeL2SnapMateInfo* m_exchange_l2_snap_mate_info;
    SMGeneratedL2TickMateInfo* m_generated_l2_tick_mate_info;
    /// Store the start/current memory address of tick area.
    OrderTick* m_order_tick_start;
    OrderTick* m_order_tick_current;
    TradeTick* m_trade_tick_start;
    TradeTick* m_trade_tick_current;
    ExchangeL2Snap* m_exchange_l2_snap_start;
    ExchangeL2Snap* m_exchange_l2_snap_current;
    GeneratedL2Tick* m_generated_l2_tick_start;
    GeneratedL2Tick* m_generated_l2_tick_current;

private:
    static constexpr boost::interprocess::offset_t GB = 1024 * 1024 * 1024;

private:
    std::shared_ptr<IReporter> m_outside;
};

} // namespace trade::reporter
