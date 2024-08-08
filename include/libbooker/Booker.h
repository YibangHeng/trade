#pragma once

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
        bool enable_validation           = false,
        bool enable_advanced_calculating = false
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
    template<typename TickTypePtr>
    void generate_level_price(const std::string& symbol, const TickTypePtr& tick);

private:
    void new_booker(const std::string& symbol);

private:
    void refresh_range(const std::string& symbol, int64_t time);
    void add_range_snap(const OrderTickPtr& order_tick);
    void add_range_snap(
        const OrderWrapperPtr& order,
        const OrderWrapperPtr& matched_order,
        liquibook::book::Quantity fill_qty,
        liquibook::book::Price fill_price
    );
    static void generate_weighted_price(
        const GeneratedL2TickPtr& latest_l2_tick,
        const GeneratedL2TickPtr& previous_l2_tick,
        const RangedTickPtr& ranged_tick
    );
    /// Align time to 3 seconds.
    /// E.g., 103000000 -> 103000000, 103001000 -> 103000000.
    static int64_t align_time(int64_t time);
    /// Minus 3 seconds from time.
    /// E.g., 103000000 -> 102957000, 103030000 -> 103027000.
    static int64_t minus_3_seconds(int64_t time);

private:
    std::unordered_set<std::string> m_failed_symbols;

private:
    /// Symbol -> GeneratedL2Tick.
    std::unordered_map<std::string, GeneratedL2TickPtr> m_generated_l2_ticks;
    /// Symbol -> RangedTick.
    std::unordered_map<std::string, std::vector<RangedTickPtr>> m_ranged_ticks;
    /// Symbol -> Previous l2 prices.
    /// Other fields in GeneratedL2Tick are not used.
    std::unordered_map<std::string, GeneratedL2TickPtr> m_privious_l2_prices;

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
    bool m_enable_advanced_calculating;

private:
    /// Symbol -> remaining quantity of market order.
    std::unordered_map<std::string, OrderTickPtr> m_market_order;

private:
    std::shared_ptr<reporter::IReporter> m_reporter;
};

template<typename TickTypePtr>
void Booker::generate_level_price(const std::string& symbol, const TickTypePtr& tick)
{
    std::array<int64_t, 5> ask_price_levels    = {};
    std::array<int64_t, 5> bid_price_levels    = {};
    std::array<int64_t, 5> ask_quantity_levels = {};
    std::array<int64_t, 5> bid_quantity_levels = {};

    /// Ask booker.
    auto ask_it = m_books[symbol]->asks().begin();

    /// Ask price/quantity levels.
    auto ask_price_level_it    = ask_price_levels.begin();
    auto ask_quantity_level_it = ask_quantity_levels.begin();

    while (ask_it != m_books[symbol]->asks().end()) {
        if (*ask_price_level_it == 0)
            *ask_price_level_it = BookerCommonData::to_price(ask_it->first.price());

        if (*ask_price_level_it != BookerCommonData::to_price(ask_it->first.price())) {
            ask_price_level_it++;
            ask_quantity_level_it++;

            if (ask_price_level_it == ask_price_levels.end()
                || ask_quantity_level_it == ask_quantity_levels.end())
                break;
        }

        *ask_price_level_it = BookerCommonData::to_price(ask_it->first.price());
        *ask_quantity_level_it += BookerCommonData::to_quantity(ask_it->second.open_qty());

        ask_it++;
    }

    /// Bid booker.
    auto bid_it = m_books[symbol]->bids().begin();

    /// Bid price/quantity levels.
    auto bid_price_level_it    = bid_price_levels.begin();
    auto bid_quantity_level_it = bid_quantity_levels.begin();

    while (bid_it != m_books[symbol]->bids().end()) {
        if (*bid_price_level_it == 0)
            *bid_price_level_it = BookerCommonData::to_price(bid_it->first.price());

        if (*bid_price_level_it != BookerCommonData::to_price(bid_it->first.price())) {
            bid_price_level_it++;
            bid_quantity_level_it++;

            if (bid_price_level_it == bid_price_levels.end()
                || bid_quantity_level_it == bid_quantity_levels.end())
                break;
        }

        *bid_price_level_it = BookerCommonData::to_price(bid_it->first.price());
        *bid_quantity_level_it += BookerCommonData::to_quantity(bid_it->second.open_qty());

        bid_it++;
    }

    tick->clear_ask_levels();
    tick->clear_bid_levels();

    /// Assign price/quantity levels to latest l2 tick.
    for (int i = 0; i < 5; i++) {
        const auto ask_level = tick->add_ask_levels();
        ask_level->set_price_1000x(ask_price_levels[i]);
        ask_level->set_quantity(ask_quantity_levels[i]);

        const auto bid_level = tick->add_bid_levels();
        bid_level->set_price_1000x(bid_price_levels[i]);
        bid_level->set_quantity(bid_quantity_levels[i]);
    }
}

} // namespace trade::booker
