#pragma once

#include <queue>

#include "OrderWrapper.h"

namespace trade::booker
{

class CallAuctionHolder
{
public:
    CallAuctionHolder()  = default;
    ~CallAuctionHolder() = default;

public:
    void push(const OrderWrapperPtr& order_wrapper);
    void trade(const types::TradeTick& trade_tick);
    std::shared_ptr<types::OrderTick> pop();

private:
    static std::shared_ptr<types::OrderTick> to_order_tick(const OrderWrapperPtr& order_wrapper);

private:
    /// unique_id -> ask order.
    std::unordered_map<int64_t, OrderWrapperPtr> m_ask_orders;
    /// unique_id -> bid order.
    std::unordered_map<int64_t, OrderWrapperPtr> m_bid_orders;
    /// To keep track of the sequence of orders.
    std::queue<int64_t> m_order_queue;
};

} // namespace trade::booker
