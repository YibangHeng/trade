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

    /// Implement the liquibook::book::order concept.
public:
    [[nodiscard]] std::string symbol() const;
    [[nodiscard]] bool is_buy() const;
    /// 0 if a market order.
    [[nodiscard]] liquibook::book::Price price() const;
    [[nodiscard]] liquibook::book::Quantity order_qty() const;

    /// 0 if not a stop order.
    /// stop_price is not used yet. Just make matcher happy.
    static consteval liquibook::book::Price stop_price() { return 0; }

private:
    std::shared_ptr<types::OrderTick> m_order;
};

using OrderWrapperPtr = std::shared_ptr<OrderWrapper>;

} // namespace trade::broker
