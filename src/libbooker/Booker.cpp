#include "libbooker/Booker.h"
#include "libbooker/BookerCommonData.h"
#include "utilities/TimeHelper.hpp"
#include "utilities/ToJSON.hpp"

trade::booker::Booker::Booker(
    const std::vector<std::string>& symbols,
    const std::shared_ptr<reporter::IReporter>& reporter
) : AppBase("Booker"),
    m_in_continuous_stage(),
    m_reporter(reporter)
{
    for (const auto& symbol : symbols)
        new_booker(symbol);
}

void trade::booker::Booker::add(const OrderTickPtr& order_tick)
{
    new_booker(order_tick->symbol());

    OrderWrapperPtr order_wrapper;

    /// Check if order already exists.
    if (m_orders.contains(order_tick->unique_id())) {
        order_wrapper = m_orders[order_tick->unique_id()];

        if (order_tick->order_type() != types::OrderType::cancel)
            logger->error("Received duplicated order {} with order type {}", order_tick->unique_id(), OrderType_Name(order_tick->order_type()));
    }
    else {
        order_wrapper = std::make_shared<OrderWrapper>(order_tick);
        m_orders.emplace(order_tick->unique_id(), order_wrapper);

        logger->info("Received order {} with order type {}", order_tick->unique_id(), OrderType_Name(order_tick->order_type()));
    }

    /// If in call auction stage.
    if (order_wrapper->exchange_time() < 925000) {
        m_call_auction_holders[order_wrapper->symbol()].push(order_wrapper);
    }
    /// If in continuous trade stage.
    else {
        auction(order_wrapper);
    }
}

void trade::booker::Booker::trade(const TradeTickPtr& trade_tick)
{
    /// SZSE reports cancel orders as trade ticks.
    /// In this case, let Booker::add() handle it.
    if (trade_tick->x_ost_szse_exe_type() == types::OrderType::cancel) [[likely]] {
        const auto order_tick = std::make_shared<types::OrderTick>();

        /// Canceling only requires unique_id.
        /// See Booker's unit tests.
        order_tick->set_unique_id(trade_tick->ask_unique_id() + trade_tick->bid_unique_id());

        add(order_tick);
    }
    /// Otherwise, only accepts trade made in auction stage.
    else if (trade_tick->exchange_time() == 925000) [[unlikely]] {
        m_call_auction_holders[trade_tick->symbol()].trade(*trade_tick);

        /// Report trade.
        const auto md_trade = std::make_shared<types::MdTrade>();

        md_trade->set_symbol(trade_tick->symbol());
        md_trade->set_price(trade_tick->exec_price());
        md_trade->set_quantity(trade_tick->exec_quantity());

        m_reporter->md_trade_generated(md_trade);
    }
}

void trade::booker::Booker::switch_to_continuous_stage()
{
    if (m_in_continuous_stage) [[likely]]
        return;

    /// Move all unfinished orders from call auction stage to continuous trade stage.
    for (auto& call_auction_holder : m_call_auction_holders | std::views::values) {
        while (true) {
            const auto order_tick = call_auction_holder.pop();
            if (order_tick == nullptr)
                break;

            logger->info("Added unfinished order in call auction stage: {}", utilities::ToJSON()(*order_tick));

            auction(std::make_shared<OrderWrapper>(order_tick));
        }
    }

    m_in_continuous_stage = true;

    logger->info("Switched to continuous trade stage at {}", utilities::Now<std::string>()());
}

void trade::booker::Booker::auction(const OrderWrapperPtr& order_wrapper)
{
    switch (order_wrapper->order_type()) {
    case types::OrderType::limit:
    case types::OrderType::market: {
        m_books[order_wrapper->symbol()]->add(order_wrapper);
        break;
    }
        /// TODO: Make the matching logic for best price clearer.
    case types::OrderType::best_price: {
        double best_price;

        if (order_wrapper->is_buy()) {
            const auto bids = m_books[order_wrapper->symbol()]->bids();

            if (bids.empty()) {
                logger->error("A best price order {} arrived while no bid exists", order_wrapper->unique_id());
                break;
            }

            best_price = BookerCommonData::to_price(bids.begin()->first.price());
        }
        else {
            const auto asks = m_books[order_wrapper->symbol()]->asks();

            if (asks.empty()) {
                logger->error("A best price order {} arrived while no ask exists", order_wrapper->unique_id());
                break;
            }

            best_price = BookerCommonData::to_price(asks.begin()->first.price());
        }

        order_wrapper->to_limit_order(best_price);

        m_books[order_wrapper->symbol()]->add(order_wrapper);
        break;
    }
    case types::OrderType::cancel: {
        m_books[order_wrapper->symbol()]->cancel(order_wrapper);
        break;
    }
    default: {
        logger->error("Unsupported order type: {}", static_cast<int>(order_wrapper->order_type()));
    }
    }
}

void trade::booker::Booker::on_trade(
    const liquibook::book::OrderBook<OrderWrapperPtr>* book,
    const liquibook::book::Quantity qty,
    const liquibook::book::Price price
)
{
    const auto md_trade = std::make_shared<types::MdTrade>();

    md_trade->set_symbol(book->symbol());
    md_trade->set_price(BookerCommonData::to_price(price));
    md_trade->set_quantity(BookerCommonData::to_quantity(qty));

    generate_level_price(book->symbol(), md_trade);

    m_reporter->md_trade_generated(md_trade);
}

void trade::booker::Booker::generate_level_price(const std::string& symbol, const std::shared_ptr<types::MdTrade>& md_trade)
{
    auto ask_it = m_books[symbol]->asks().begin();
    auto bid_it = m_books[symbol]->bids().begin();

    if (ask_it != m_books[symbol]->asks().end()) md_trade->set_sell_1(BookerCommonData::to_price(ask_it++->first.price()));
    if (ask_it != m_books[symbol]->asks().end()) md_trade->set_sell_2(BookerCommonData::to_price(ask_it++->first.price()));
    if (ask_it != m_books[symbol]->asks().end()) md_trade->set_sell_3(BookerCommonData::to_price(ask_it++->first.price()));
    if (ask_it != m_books[symbol]->asks().end()) md_trade->set_sell_4(BookerCommonData::to_price(ask_it++->first.price()));
    if (ask_it != m_books[symbol]->asks().end()) md_trade->set_sell_5(BookerCommonData::to_price(ask_it++->first.price()));
    if (ask_it != m_books[symbol]->asks().end()) md_trade->set_sell_6(BookerCommonData::to_price(ask_it++->first.price()));
    if (ask_it != m_books[symbol]->asks().end()) md_trade->set_sell_7(BookerCommonData::to_price(ask_it++->first.price()));
    if (ask_it != m_books[symbol]->asks().end()) md_trade->set_sell_8(BookerCommonData::to_price(ask_it++->first.price()));
    if (ask_it != m_books[symbol]->asks().end()) md_trade->set_sell_9(BookerCommonData::to_price(ask_it++->first.price()));
    if (ask_it != m_books[symbol]->asks().end()) md_trade->set_sell_10(BookerCommonData::to_price(ask_it++->first.price()));

    if (bid_it != m_books[symbol]->bids().end()) md_trade->set_buy_1(BookerCommonData::to_price(bid_it++->first.price()));
    if (bid_it != m_books[symbol]->bids().end()) md_trade->set_buy_2(BookerCommonData::to_price(bid_it++->first.price()));
    if (bid_it != m_books[symbol]->bids().end()) md_trade->set_buy_3(BookerCommonData::to_price(bid_it++->first.price()));
    if (bid_it != m_books[symbol]->bids().end()) md_trade->set_buy_4(BookerCommonData::to_price(bid_it++->first.price()));
    if (bid_it != m_books[symbol]->bids().end()) md_trade->set_buy_5(BookerCommonData::to_price(bid_it++->first.price()));
    if (bid_it != m_books[symbol]->bids().end()) md_trade->set_buy_6(BookerCommonData::to_price(bid_it++->first.price()));
    if (bid_it != m_books[symbol]->bids().end()) md_trade->set_buy_7(BookerCommonData::to_price(bid_it++->first.price()));
    if (bid_it != m_books[symbol]->bids().end()) md_trade->set_buy_8(BookerCommonData::to_price(bid_it++->first.price()));
    if (bid_it != m_books[symbol]->bids().end()) md_trade->set_buy_9(BookerCommonData::to_price(bid_it++->first.price()));
    if (bid_it != m_books[symbol]->bids().end()) md_trade->set_buy_10(BookerCommonData::to_price(bid_it++->first.price()));
}

void trade::booker::Booker::on_reject(const OrderWrapperPtr& order, const char* reason)
{
    logger->error("Order {} was rejected: {}", order->unique_id(), reason);
}

void trade::booker::Booker::on_cancel_reject(const OrderWrapperPtr& order, const char* reason)
{
    logger->error("Cancel for {} was rejected: {}", order->unique_id(), reason);
}

void trade::booker::Booker::on_replace_reject(const OrderWrapperPtr& order, const char* reason)
{
    logger->error("Replace for {} was rejected: {}", order->unique_id(), reason);
}

void trade::booker::Booker::new_booker(const std::string& symbol)
{
    /// Do nothing if the order book already exists.
    if (m_books.contains(symbol)) [[likely]]
        return;

    m_books.emplace(symbol, std::make_shared<liquibook::book::OrderBook<OrderWrapperPtr>>(symbol));
    m_call_auction_holders.emplace(symbol, CallAuctionHolder());

    m_books[symbol]->set_symbol(symbol);
    m_books[symbol]->set_trade_listener(this);

    logger->info("Created new order book for symbol {}", symbol);
}
