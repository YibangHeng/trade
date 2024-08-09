#pragma once

#include <third/liquibook/src/book/order_book.h>

#include "BookerCommonData.h"

namespace trade::booker
{

/// Wrapper types::Order for liquibook::book::Order.
class OrderWrapper
{
public:
    explicit OrderWrapper(const OrderTickPtr& order_tick);
    ~OrderWrapper() = default;

public:
    [[nodiscard]] int64_t unique_id() const;

public:
    /// Convert order type to limit from best price.
    ///
    /// @param price The latest price.
    ///
    /// @note A best price order acts like a limit order that automatically
    /// fills with the latest best market price with side matching the order's
    /// side.
    void to_limit_order(int64_t price) const;

    /// Implement the liquibook::book::order concept.
public:
    [[nodiscard]] std::string symbol() const;
    [[nodiscard]] bool is_buy() const;
    /// 0 if a market order.
    [[nodiscard]] liquibook::book::Price price() const;
    [[nodiscard]] liquibook::book::Quantity order_qty() const;
    [[nodiscard]] bool is_limit() const;

    /// 0 if not a stop order.
    /// stop_price is not used yet. Just make booker happy.
    static consteval liquibook::book::Price stop_price() { return 0; }

public:
    /// Accept a trade on this order and return true if the order is filled.
    bool accept(const types::TradeTick& trade_tick);

public:
    [[nodiscard]] types::OrderType order_type() const;
    [[nodiscard]] int64_t exchange_date() const;
    [[nodiscard]] int64_t exchange_time() const;
    /// Return the quantity that not yet filled.
    [[nodiscard]] liquibook::book::Quantity quantity_on_market() const;
    [[nodiscard]] OrderTickPtr raw_order_tick() const;

public:
    void mark_as_cancel(int64_t exchange_time) const;

private:
    OrderTickPtr m_order;
    liquibook::book::Quantity filled_quantity;
};

using OrderWrapperPtr = std::shared_ptr<OrderWrapper>;

} // namespace trade::booker
