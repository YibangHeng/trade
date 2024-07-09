#pragma once

#include <boost/circular_buffer.hpp>
#include <boost/functional/hash.hpp>

#include "BookerCommonData.h"
#include "networks.pb.h"

namespace trade::booker
{

class TradeTickHash
{
public:
    using HashType = decltype(std::hash<std::string>()(""));

    HashType operator()(const types::TradeTick& trade_tick) const
    {
        HashType seed = 0;

        boost::hash_combine(seed, std::hash<double>()(trade_tick.exec_price()));
        boost::hash_combine(seed, std::hash<int64_t>()(trade_tick.exec_quantity()));

        return seed;
    }

    /// Trade ticks are fed by l2 ticks. Therefore, TradeTickHash should accept
    /// l2 ticks.
    HashType operator()(const types::L2Tick& l2_tick) const
    {
        HashType seed = 0;

        boost::hash_combine(seed, std::hash<double>()(l2_tick.price()));
        boost::hash_combine(seed, std::hash<int64_t>()(l2_tick.quantity()));

        return seed;
    }
};

class L2TickHash
{
public:
    using HashType = decltype(std::hash<double>()(0.));

    HashType operator()(const types::L2Tick& l2_tick) const
    {
        HashType seed = 0;

        boost::hash_combine(seed, std::hash<double>()(l2_tick.sell_price_1()));
        boost::hash_combine(seed, std::hash<int64_t>()(l2_tick.sell_quantity_1()));
        boost::hash_combine(seed, std::hash<double>()(l2_tick.sell_price_2()));
        boost::hash_combine(seed, std::hash<int64_t>()(l2_tick.sell_quantity_2()));
        boost::hash_combine(seed, std::hash<double>()(l2_tick.sell_price_3()));
        boost::hash_combine(seed, std::hash<int64_t>()(l2_tick.sell_quantity_3()));
        boost::hash_combine(seed, std::hash<double>()(l2_tick.buy_price_1()));
        boost::hash_combine(seed, std::hash<int64_t>()(l2_tick.buy_quantity_1()));
        boost::hash_combine(seed, std::hash<double>()(l2_tick.buy_price_2()));
        boost::hash_combine(seed, std::hash<int64_t>()(l2_tick.buy_quantity_2()));
        boost::hash_combine(seed, std::hash<double>()(l2_tick.buy_price_3()));
        boost::hash_combine(seed, std::hash<int64_t>()(l2_tick.buy_quantity_3()));

        return seed;
    }
};

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
    /// Check by exchange l2 tick.
    bool check(const L2TickPtr& l2_tick) const;

private:
    void new_trade_tick_buffer(const std::string& symbol);
    void new_l2_tick_buffer(const std::string& symbol);

private:
    constexpr static int m_buffer_size = 1024; /// Last 1024 ticks.
    /// Symbol -> traded ask/bid unique ids.
    std::unordered_map<std::string, boost::circular_buffer<int64_t>> m_traded_ask_unique_ids;
    std::unordered_map<std::string, boost::circular_buffer<int64_t>> m_traded_bid_unique_ids;
    /// Symbol -> l2 ticks.
    std::unordered_map<std::string, boost::circular_buffer<L2TickHash::HashType>> m_l2_tick_buffers;
};

} // namespace trade::booker
