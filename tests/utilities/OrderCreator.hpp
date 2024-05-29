#pragma once

#include "networks.pb.h"

class OrderCreator
{
public:
    static std::shared_ptr<trade::types::OrderTick> ororder_tick(
        const int64_t unique_id,
        const trade::types::OrderType order_type = trade::types::OrderType::invalid_order_type,
        std::string symbol                       = "",
        const trade::types::SideType side        = trade::types::SideType::invalid_side,
        const double price                       = 0.0,
        const int64_t quantity                   = 0
    )
    {
        auto order_tick = std::make_shared<trade::types::OrderTick>();

        order_tick->set_unique_id(unique_id);
        order_tick->set_order_type(order_type);
        order_tick->set_symbol(symbol);
        order_tick->set_side(side);
        order_tick->set_price(price);
        order_tick->set_quantity(quantity);

        return order_tick;
    }

    static std::shared_ptr<trade::types::TradeTick> trade_tick(
        const int64_t ask_unique_id,
        const int64_t bid_unique_id,
        std::string symbol,
        const double exec_price,
        const int64_t exec_quantity,
        const int64_t exchange_time
    )
    {
        auto trade_tick = std::make_shared<trade::types::TradeTick>();

        trade_tick->set_ask_unique_id(ask_unique_id);
        trade_tick->set_bid_unique_id(bid_unique_id);
        trade_tick->set_symbol(symbol);
        trade_tick->set_exec_price(exec_price);
        trade_tick->set_exec_quantity(exec_quantity);
        trade_tick->set_exchange_time(exchange_time);

        return trade_tick;
    }

    static trade::booker::OrderWrapperPtr order_wrapper(
        const int64_t unique_id,
        const trade::types::OrderType order_type = trade::types::OrderType::invalid_order_type,
        const std::string& symbol                = "",
        const trade::types::SideType side        = trade::types::SideType::invalid_side,
        const double price                       = 0.0,
        const int64_t quantity                   = 0
    )
    {
        return std::make_shared<trade::booker::OrderWrapper>(ororder_tick(unique_id, order_type, symbol, side, price, quantity));
    }
};

#define LIMIT trade::types::OrderType::limit
#define MARKET trade::types::OrderType::market
#define BEST_PRICE trade::types::OrderType::best_price
#define CANCEL trade::types::OrderType::cancel

#define BUY trade::types::SideType::buy
#define SELL trade::types::SideType::sell

/// TODO: Not enabled yet for .clang-format reasons.
#define T(symbol, price, quantity) #symbol ":" #price ":" #quantity "\n"
#define M(symbol, price) #symbol ":" #price "\n"
