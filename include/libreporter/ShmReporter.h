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

struct PUBLIC_API SMMarketDataMateInfo {
    size_t market_data_count  = 0;
    char last_update_time[32] = {}; /// Time in ISO 8601 format.

private:
    u_char m_reserved[216] = {}; /// For aligning of cache line.
};

static_assert(sizeof(SMMarketDataMateInfo) == 256, "MateInfo should be 256 bytes");

struct PriceQuantityPair {
    double price     = 0;
    int64_t quantity = 0;
};

enum class ShmUnionType
{
    invalid_shm_union          = 0,

    tick_from_exchange         = 1, /// 交易所逐笔委托
    market_data_from_exchange  = 2, /// 交易所行情切片
    self_generated_market_data = 3, /// 撮合行情
};

static_assert(sizeof(ShmUnionType) == 4, "ShmUnionType should be 4 bytes");

struct PUBLIC_API L2TickData {
    ShmUnionType shm_union_type = ShmUnionType::self_generated_market_data;

    char symbol[16]             = {};
    double price                = 0;
    int32_t quantity            = 0;

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

private:
    u_char m_reserved[160] = {}; /// For aligning of cache line.
};

static_assert(sizeof(L2Tick) == 512, "L2Tick should be 512 bytes");

union PUBLIC_API ShmUnion
{
    L2Tick market_data;
};

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
    void l2_tick_generated(std::shared_ptr<types::L2Tick> l2_tick) override;

private:
    /// Return the start memory address of trade area.
    boost::interprocess::offset_t trade_offset();

private:
    boost::interprocess::shared_memory_object m_shm;
    std::shared_ptr<boost::interprocess::mapped_region> m_tick_region;
    std::shared_ptr<boost::interprocess::mapped_region> m_market_data_region;
    boost::interprocess::named_upgradable_mutex m_named_mutex;
    SMMarketDataMateInfo* m_market_data_mate_info;
    /// Store the start memory address of market data area.
    L2Tick* m_market_data_start;
    /// Store the current memory address of market data area, which is used for next writing.
    L2Tick* m_market_data_current;

private:
    static constexpr boost::interprocess::offset_t GB = 1024 * 1024 * 1024;

private:
    std::shared_ptr<IReporter> m_outside;
};

} // namespace trade::reporter
