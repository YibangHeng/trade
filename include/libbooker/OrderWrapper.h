#pragma once

#include <third/liquibook/src/book/order_book.h>

#include "orms.pb.h"

namespace trade::broker
{

/// Wrapper types::Order for liquibook::book::Order.
class OrderWrapper
{
public:
    explicit OrderWrapper(const std::shared_ptr<types::OrderTick>& order_tick);
    ~OrderWrapper() = default;

public:
    [[nodiscard]] int64_t unique_id() const;

public:
    /// Convert order type to limit from best price.
    ///
    /// @param price The latest best market price.
    ///
    /// @note A best price order acts like a limit order that automatically
    /// fills with the latest best market price with side matching the order's
    /// side.
    void to_limit_order(double price) const;

    /// Implement the liquibook::book::order concept.
public:
    [[nodiscard]] std::string symbol() const;
    [[nodiscard]] bool is_buy() const;
    /// 0 if a market order.
    [[nodiscard]] liquibook::book::Price price() const;
    [[nodiscard]] liquibook::book::Quantity order_qty() const;
    [[nodiscard]] bool is_limit() const;

public:
    [[nodiscard]] types::OrderType order_type() const;
    [[nodiscard]] bool exchange_time() const;

    /// 0 if not a stop order.
    /// stop_price is not used yet. Just make matcher happy.
    static consteval liquibook::book::Price stop_price() { return 0; }

private:
    std::shared_ptr<types::OrderTick> m_order;
};

using OrderWrapperPtr = std::shared_ptr<OrderWrapper>;

} // namespace trade::broker
