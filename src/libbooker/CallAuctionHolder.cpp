#include "libbooker/CallAuctionHolder.h"
#include "libbooker/BookerCommonData.h"

void trade::booker::CallAuctionHolder::push(const OrderWrapperPtr& order_wrapper)
{
    if (order_wrapper->order_type() == types::OrderType::cancel) {
        order_wrapper->is_buy()
            ? m_bid_orders.erase(order_wrapper->unique_id())
            : m_ask_orders.erase(order_wrapper->unique_id());
        return;
    }

    order_wrapper->is_buy()
        ? m_bid_orders.emplace(order_wrapper->unique_id(), order_wrapper)
        : m_ask_orders.emplace(order_wrapper->unique_id(), order_wrapper);

    m_order_queue.push(order_wrapper->unique_id());
}

void trade::booker::CallAuctionHolder::trade(const types::TradeTick& trade_tick)
{
    const auto ask_order = m_ask_orders.find(trade_tick.ask_unique_id());
    const auto bid_order = m_bid_orders.find(trade_tick.bid_unique_id());

    if (ask_order == m_ask_orders.end() || bid_order == m_bid_orders.end())
        return;

    if (ask_order->second->accept(trade_tick))
        m_ask_orders.erase(ask_order);

    if (bid_order->second->accept(trade_tick))
        m_bid_orders.erase(bid_order);
}

std::shared_ptr<trade::types::OrderTick> trade::booker::CallAuctionHolder::pop()
{
    while (!m_order_queue.empty()) {
        const auto next = m_order_queue.front();
        m_order_queue.pop();

        std::unordered_map<int64_t, OrderWrapperPtr>::iterator order;

        if (order = m_bid_orders.find(next); order != m_bid_orders.end()) {
            const auto order_tick = to_order_tick(order->second);
            m_bid_orders.erase(order);
            return order_tick;
        }
        if (order = m_ask_orders.find(next); order != m_ask_orders.end()) {
            const auto order_tick = to_order_tick(order->second);
            m_ask_orders.erase(order);
            return order_tick;
        }
    }

    return nullptr;
}

std::shared_ptr<trade::types::OrderTick> trade::booker::CallAuctionHolder::to_order_tick(const OrderWrapperPtr& order_wrapper)
{
    const auto order_tick = std::make_shared<types::OrderTick>();

    order_tick->set_unique_id(order_wrapper->unique_id());
    order_tick->set_order_type(order_wrapper->order_type());
    order_tick->set_symbol(order_wrapper->symbol());
    order_tick->set_side(order_wrapper->is_buy() ? types::SideType::buy : types::SideType::sell);
    order_tick->set_price_1000x(BookerCommonData::to_price(order_wrapper->price()));
    order_tick->set_quantity(BookerCommonData::to_quantity(order_wrapper->quantity_on_market()));
    order_tick->set_exchange_time(order_wrapper->exchange_time());

    return order_tick;
}
