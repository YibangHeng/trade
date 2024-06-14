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

struct PUBLIC_API SMMateInfo {
    size_t market_data_count  = 0;
    char last_update_time[32] = {}; /// Time in ISO 8601 format.

private:
    u_char m_reserved[216] = {}; /// For aligning of cache line.
};

static_assert(sizeof(SMMateInfo) == 256, "MateInfo should be 256 bytes");

struct PUBLIC_API SMMarketData {
    char symbol[16]  = {};
    double price     = 0;
    int32_t quantity = 0;

    double sell_10   = 0;
    double sell_9    = 0;
    double sell_8    = 0;
    double sell_7    = 0;
    double sell_6    = 0;
    double sell_5    = 0;
    double sell_4    = 0;
    double sell_3    = 0;
    double sell_2    = 0;
    double sell_1    = 0;
    double buy_1     = 0;
    double buy_2     = 0;
    double buy_3     = 0;
    double buy_4     = 0;
    double buy_5     = 0;
    double buy_6     = 0;
    double buy_7     = 0;
    double buy_8     = 0;
    double buy_9     = 0;
    double buy_10    = 0;

private:
    u_char m_reserved[68] = {}; /// For aligning of cache line.
};

static_assert(sizeof(SMMarketData) == 256, "SMTrade should be 256 bytes");

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
    void md_trade_generated(std::shared_ptr<types::MdTrade> md_trade) override;

private:
    /// Return the start memory address of trade area.
    boost::interprocess::offset_t trade_offset();

private:
    boost::interprocess::shared_memory_object m_md_shm;
    std::shared_ptr<boost::interprocess::mapped_region> m_md_region;
    boost::interprocess::named_upgradable_mutex m_named_mutex;
    SMMateInfo* m_shm_mate_info;
    /// Store the start memory address of market data area.
    SMMarketData* m_md_start;
    /// Store the current memory address of market data area, which is used for next writing.
    SMMarketData* m_md_current;

private:
    static constexpr boost::interprocess::offset_t GB = 1024 * 1024 * 1024;

private:
    std::shared_ptr<IReporter> m_outside;
};

} // namespace trade::reporter
