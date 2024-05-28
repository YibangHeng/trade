#pragma once

#include <memory>
#include <third/liquibook/src/book/order_book.h>

#include "AppBase.hpp"
#include "OrderWrapper.h"
#include "Rearranger.h"
#include "libreporter/IReporter.hpp"
#include "visibility.h"

namespace trade::broker
{

using OrderBook    = liquibook::book::OrderBook<OrderWrapperPtr>;
using OrderBookPtr = std::shared_ptr<OrderBook>;

class PUBLIC_API Booker final: AppBase<>,
                               private OrderBook::TypedTradeListener,
                               private OrderBook::TypedOrderListener
{
public:
    explicit Booker(
        const std::vector<std::string>& symbols,
        const std::shared_ptr<reporter::IReporter>& reporter
    );
    ~Booker() override = default;

public:
    void add(const std::shared_ptr<types::OrderTick>& order_tick);

private:
    void on_trade(
        const liquibook::book::OrderBook<std::shared_ptr<OrderWrapper>>* book,
        liquibook::book::Quantity qty,
        liquibook::book::Price price
    ) override;
    void generate_level_price(const std::string& symbol);

private:
    void on_accept(const std::shared_ptr<OrderWrapper>& order) override {}
    void on_trigger_stop(const std::shared_ptr<OrderWrapper>& order) override {}
    void on_reject(const std::shared_ptr<OrderWrapper>& order, const char* reason) override;
    void on_fill(const std::shared_ptr<OrderWrapper>& order, const std::shared_ptr<OrderWrapper>& matched_order, liquibook::book::Quantity fill_qty, liquibook::book::Price fill_price) override {}
    void on_cancel(const std::shared_ptr<OrderWrapper>& order) override {}
    void on_cancel_reject(const std::shared_ptr<OrderWrapper>& order, const char* reason) override;
    void on_replace(const std::shared_ptr<OrderWrapper>& order, const int64_t& size_delta, liquibook::book::Price new_price) override {}
    void on_replace_reject(const std::shared_ptr<OrderWrapper>& order, const char* reason) override;

private:
    void new_booker(const std::string& symbol);

private:
    /// Symbol -> OrderBook.
    std::unordered_map<std::string, OrderBookPtr> books;
    /// Symbol -> Rearranger.
    std::unordered_map<std::string, Rearranger> rearrangers;
    /// TODO: Use a better way to cache orders.
    std::unordered_map<int64_t, std::shared_ptr<OrderWrapper>> orders;
    std::array<double, reporter::level_depth> asks;
    std::array<double, reporter::level_depth> bids;

private:
    std::shared_ptr<reporter::IReporter> m_reporter;
};

} // namespace trade::broker
