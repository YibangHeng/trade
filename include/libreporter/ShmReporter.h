#pragma once

#include <boost/interprocess/mapped_region.hpp>
#include <boost/interprocess/shared_memory_object.hpp>
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

struct PUBLIC_API SMTrade {
    char symbol[16]  = {};
    double price     = 0;
    int32_t quantity = 0;

private:
    u_char m_reserved[4] = {}; /// For aligning of cache line.
};

static_assert(sizeof(SMTrade) == 32, "SMTrade should be 32 bytes");

struct PUBLIC_API SMMarketPrice {
    char symbol[16] = {};
    double price    = 0;

private:
    u_char m_reserved[8] = {};
};

static_assert(sizeof(SMMarketPrice) == 32, "SMMarketPrice should be 32 bytes");

struct PUBLIC_API SMLevelPrice {
    char symbol[16] = {};
    double sell_9   = 0;
    double sell_8   = 0;
    double sell_7   = 0;
    double sell_6   = 0;
    double sell_5   = 0;
    double sell_4   = 0;
    double sell_3   = 0;
    double sell_2   = 0;
    double sell_1   = 0;
    double sell_0   = 0;
    double buy_0    = 0;
    double buy_1    = 0;
    double buy_2    = 0;
    double buy_3    = 0;
    double buy_4    = 0;
    double buy_5    = 0;
    double buy_6    = 0;
    double buy_7    = 0;
    double buy_8    = 0;
    double buy_9    = 0;

private:
    u_char m_reserved[80] = {};
};

static_assert(sizeof(SMLevelPrice) == 256, "SMLevelPrice should be 256 bytes");

#pragma pack(pop)

/// ShmReporter writes market data to shared memory.
class PUBLIC_API ShmReporter final: private AppBase<>, public NopReporter
{
public:
    explicit ShmReporter(std::shared_ptr<IReporter> outside = std::make_shared<NopReporter>());
    ~ShmReporter() override = default;

    /// Market data.
public:
    void md_trade_generated(std::shared_ptr<types::MdTrade> md_trade) override;
    void market_price(std::string symbol, double price) override;
    void level_price(std::array<double, level_depth> asks, std::array<double, level_depth> bids) override;

private:
    /// Return the start memory address of trade area.
    boost::interprocess::offset_t trade_offset();

private:
    boost::interprocess::shared_memory_object m_shm;
    std::shared_ptr<boost::interprocess::mapped_region> m_trade_region;
    std::shared_ptr<boost::interprocess::mapped_region> m_market_price_region;
    std::shared_ptr<boost::interprocess::mapped_region> m_level_price_region;
    SMTrade* m_trade_start;
    SMMarketPrice* m_market_price_start;
    SMLevelPrice* m_level_price_start;

private:
    static constexpr boost::interprocess::offset_t GB = 1024 * 1024 * 1024;

private:
    std::shared_ptr<IReporter> m_outside;
};

} // namespace trade::reporter
