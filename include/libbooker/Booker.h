#pragma once

#include <boost/circular_buffer.hpp>
#include <memory>
#include <optional>
#include <third/liquibook/src/book/order_book.h>

#include "AppBase.hpp"
#include "CallAuctionHolder.h"
#include "MdValidator.h"
#include "OrderWrapper.h"
#include "libreporter/IReporter.hpp"
#include "visibility.h"

namespace trade::booker
{

using OrderBook    = liquibook::book::OrderBook<OrderWrapperPtr>;
using OrderBookPtr = std::shared_ptr<OrderBook>;

template<typename T>
struct BuySellPair {
    T buy;
    T sell;
};

class PUBLIC_API Booker final: AppBase<>,
                               private OrderBook::TypedTradeListener,
                               private OrderBook::TypedOrderListener
{
public:
    Booker(
        const std::vector<std::string>& symbols,
        const std::shared_ptr<reporter::IReporter>& reporter,
        bool enable_validation = false
    );
    ~Booker() override = default;

public:
    /// Add a new order/cancel to the book.
    /// Booker will process order in auction stage and continuous stage.
    void add(const OrderTickPtr& order_tick);
    bool trade(const TradeTickPtr& trade_tick);
    void switch_to_continuous_stage();

private:
    void auction(const OrderWrapperPtr& order_wrapper);

private:
    void on_trade(
        const liquibook::book::OrderBook<OrderWrapperPtr>* book,
        liquibook::book::Quantity qty,
        liquibook::book::Price price
    ) override;

private:
    void on_accept(const OrderWrapperPtr& order) override {}
    void on_trigger_stop(const OrderWrapperPtr& order) override {}
    void on_reject(const OrderWrapperPtr& order, const char* reason) override;
    void on_fill(
        const OrderWrapperPtr& order,
        const OrderWrapperPtr& matched_order,
        liquibook::book::Quantity fill_qty,
        liquibook::book::Price fill_price
    ) override;
    void on_cancel(const OrderWrapperPtr& order) override {}
    void on_cancel_reject(const OrderWrapperPtr& order, const char* reason) override;
    void on_replace(const OrderWrapperPtr& order, const int64_t& size_delta, liquibook::book::Price new_price) override {}
    void on_replace_reject(const OrderWrapperPtr& order, const char* reason) override;

private:
    static OrderTickPtr create_virtual_sse_order_tick(const TradeTickPtr& trade_tick, types::SideType side);
    OrderTickPtr create_virtual_szse_order_tick(const TradeTickPtr& trade_tick);

private:
    void generate_level_price(
        const std::string& symbol,
        const GeneratedL2TickPtr& latest_l2_tick
    );
    static void generate_weighted_price(
        const GeneratedL2TickPtr& latest_l2_tick,
        const GeneratedL2TickPtr& previous_l2_tick
    );
    void generate_statistic_data_on_order(const OrderWrapperPtr& order);
    void generate_statistic_data_on_trade(
        const OrderWrapperPtr& order,
        const OrderWrapperPtr& matched_order,
        liquibook::book::Quantity fill_qty,
        liquibook::book::Price fill_price
    );

private:
    void new_booker(const std::string& symbol);

private:
    std::unordered_map<std::string, boost::circular_buffer<GeneratedL2TickPtr>> m_l2_ticks;
    std::unordered_set<std::string> m_failed_symbols;

private:
    /// Symbol -> OrderBook.
    std::unordered_map<std::string, OrderBookPtr> m_books;
    /// Symbol -> CallAuctionHolder.
    std::unordered_map<std::string, CallAuctionHolder> m_call_auction_holders;
    /// TODO: Use a better way to cache orders.
    std::unordered_map<int64_t, OrderWrapperPtr> m_orders;
    /// Indicates if the book is in call auction stage or in continuous trade stage.
    bool m_in_continuous_stage;
    std::optional<MdValidator> m_md_validator;

private:
    /// Symbol -> remaining quantity of market order.
    std::unordered_map<std::string, OrderTickPtr> m_market_order;

private:
    std::shared_ptr<reporter::IReporter> m_reporter;
};

} // namespace trade::booker
