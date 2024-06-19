#pragma once

#include <memory>
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
    /// Add a new order/cancel to the book.
    /// Booker will process order in auction stage and continuous stage.
    void add(const OrderTickPtr& order_tick);
    /// Accept trades in call auction stage (whose exec time == 925000).
    ///
    /// Also accept SZSE new cancel.
    /// SZSE reports cancel orders as trade ticks. In this case, let
    /// Booker::add() handle it.
    void trade(const TradeTickPtr& trade_tick);
    /// Accept l2 md info, whilch will be used for checking the self-generated
    /// md info.
    bool l2(const MdTradePtr& md_trade) const;
    void switch_to_continuous_stage();

private:
    void auction(const OrderWrapperPtr& order_wrapper);

private:
    void on_trade(
        const liquibook::book::OrderBook<OrderWrapperPtr>* book,
        liquibook::book::Quantity qty,
        liquibook::book::Price price
    ) override;
    void generate_level_price(const std::string& symbol, const std::shared_ptr<types::MdTrade>& md_trade);

private:
    void on_accept(const OrderWrapperPtr& order) override {}
    void on_trigger_stop(const OrderWrapperPtr& order) override {}
    void on_reject(const OrderWrapperPtr& order, const char* reason) override;
    void on_fill(const OrderWrapperPtr& order, const OrderWrapperPtr& matched_order, liquibook::book::Quantity fill_qty, liquibook::book::Price fill_price) override {}
    void on_cancel(const OrderWrapperPtr& order) override {}
    void on_cancel_reject(const OrderWrapperPtr& order, const char* reason) override;
    void on_replace(const OrderWrapperPtr& order, const int64_t& size_delta, liquibook::book::Price new_price) override {}
    void on_replace_reject(const OrderWrapperPtr& order, const char* reason) override;

private:
    void new_booker(const std::string& symbol);

private:
    /// Symbol -> OrderBook.
    std::unordered_map<std::string, OrderBookPtr> m_books;
    /// Symbol -> CallAuctionHolder.
    std::unordered_map<std::string, CallAuctionHolder> m_call_auction_holders;
    /// TODO: Use a better way to cache orders.
    std::unordered_map<int64_t, OrderWrapperPtr> m_orders;
    /// Indicates if the book is in call auction stage or in continuous trade stage.
    bool m_in_continuous_stage;
    MdValidator m_md_validator;

private:
    std::shared_ptr<reporter::IReporter> m_reporter;
};

} // namespace trade::booker
