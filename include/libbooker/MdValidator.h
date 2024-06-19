#pragma once

#include <boost/circular_buffer.hpp>

#include "BookerCommonData.h"
#include "networks.pb.h"

namespace trade::booker
{

class MdTradeHash
{
public:
    int64_t operator()(const types::MdTrade& md_trade) const
    {
        return std::llround(
            md_trade.sell_price_5() * 10000000000000
            + md_trade.sell_price_4() * 1000000000000
            + md_trade.sell_price_3() * 100000000000
            + md_trade.sell_price_2() * 10000000000
            + md_trade.sell_price_1() * 1000000000
            + md_trade.buy_price_5() * 100000000
            + md_trade.buy_price_4() * 10000000
            + md_trade.buy_price_3() * 1000000
            + md_trade.buy_price_2() * 100000
            + md_trade.buy_price_1() * 10000
        );
    }
};

class MdValidator
{
public:
    explicit MdValidator() = default;
    ~MdValidator()         = default;

    /// Market data.
public:
    void md_trade_generated(const std::shared_ptr<types::MdTrade>& md_trade);

public:
    bool check(const MdTradePtr& md_trade) const;

private:
    void new_md_trade_buffer(const std::string& symbol);

private:
    /// Symbol -> trades.
    std::unordered_map<std::string, boost::circular_buffer<int64_t>> m_md_trade_buffers;
    constexpr static int m_buffer_size = 1000; /// Last 1000 trades.
};

} // namespace trade::booker
