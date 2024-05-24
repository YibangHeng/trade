#pragma once

#include <memory>
#include <third/liquibook/src/book/order_book.h>

#include "AppBase.hpp"
#include "OrderWrapper.h"
#include "libreporter/IReporter.hpp"
#include "visibility.h"

namespace trade::broker
{

using OrderBook    = liquibook::book::OrderBook<OrderWrapperPtr>;
using OrderBookPtr = std::shared_ptr<OrderBook>;

class PUBLIC_API Booker final: AppBase<>, private OrderBook::TypedTradeListener
{
public:
    explicit Booker(
        const std::vector<std::string>& symbols,
        const std::shared_ptr<reporter::IReporter>& reporter
    );
    ~Booker() = default;

public:
    void add(const std::shared_ptr<types::OrderTick>& order_tick);

private:
    void on_trade(
        const liquibook::book::OrderBook<std::shared_ptr<OrderWrapper>>* book,
        liquibook::book::Quantity qty,
        liquibook::book::Price price
    ) override;

private:
    /// Symbol -> OrderBook.
    std::unordered_map<std::string, OrderBookPtr> books;

private:
    std::shared_ptr<reporter::IReporter> m_reporter;
};

} // namespace trade::broker
