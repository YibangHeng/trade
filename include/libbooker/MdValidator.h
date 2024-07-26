#pragma once

#include <boost/circular_buffer.hpp>
#include <boost/functional/hash.hpp>

#include "BookerCommonData.h"
#include "networks.pb.h"

namespace trade::booker
{

class MdValidator
{
public:
    explicit MdValidator() = default;
    ~MdValidator()         = default;

    /// Market data.
public:
    /// Feed self-generated l2 tick.
    void l2_tick_generated(const L2TickPtr& l2_tick);

public:
    /// Check by exchange trade tick.
    bool check(const TradeTickPtr& trade_tick) const;

private:
    constexpr static int m_buffer_size = 1024; /// Last 1024 ticks.
    /// Symbol -> traded ask/bid unique ids.
    std::unordered_map<std::string, int64_t> m_traded_ask_unique_id;
    std::unordered_map<std::string, int64_t> m_traded_bid_unique_id;
    std::unordered_map<std::string, int64_t> m_traded_quantity;
};

} // namespace trade::booker
