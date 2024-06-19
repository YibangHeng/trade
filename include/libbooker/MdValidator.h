#pragma once

#include <boost/circular_buffer.hpp>

#include "BookerCommonData.h"
#include "networks.pb.h"

namespace trade::booker
{

class MdTradeHash
{
public:
    using HashType = decltype(std::hash<double>()(0.));

    HashType operator()(const types::L2Tick& l2_tick) const
    {
        const std::size_t sp1 = std::hash<double>()(l2_tick.sell_price_1());
        const std::size_t sp2 = std::hash<double>()(l2_tick.sell_price_2());
        const std::size_t sp3 = std::hash<double>()(l2_tick.sell_price_3());
        const std::size_t sp4 = std::hash<double>()(l2_tick.sell_price_4());
        const std::size_t sp5 = std::hash<double>()(l2_tick.sell_price_5());

        const std::size_t bp1 = std::hash<double>()(l2_tick.buy_price_1());
        const std::size_t bp2 = std::hash<double>()(l2_tick.buy_price_2());
        const std::size_t bp3 = std::hash<double>()(l2_tick.buy_price_3());
        const std::size_t bp4 = std::hash<double>()(l2_tick.buy_price_4());
        const std::size_t bp5 = std::hash<double>()(l2_tick.buy_price_5());

        return sp1 ^ sp2 << 1 ^ sp3 << 2 ^ sp4 << 3 ^ sp5 << 4 ^ bp1 << 5 ^ bp2 << 6 ^ bp3 << 7 ^ bp4 << 8 ^ bp5 << 9;
    }
};

class MdValidator
{
public:
    explicit MdValidator() = default;
    ~MdValidator()         = default;

    /// Market data.
public:
    void l2_tick_generated(const std::shared_ptr<types::L2Tick>& l2_tick);

public:
    bool check(const L2TickPtr& l2_tick) const;

private:
    void new_l2_tick_buffer(const std::string& symbol);

private:
    /// Symbol -> trades.
    std::unordered_map<std::string, boost::circular_buffer<MdTradeHash::HashType>> m_l2_tick_buffers;
    constexpr static int m_buffer_size = 1000; /// Last 1000 trades.
};

} // namespace trade::booker
