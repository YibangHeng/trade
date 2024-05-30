#include "libbooker/Booker.h"
#include "libbooker/BookerCommonData.h"
#include "utilities/TimeHelper.hpp"
#include "utilities/ToJSON.hpp"

trade::booker::Booker::Booker(
    const std::vector<std::string>& symbols,
    const std::shared_ptr<reporter::IReporter>& reporter
) : AppBase("Booker"),
    m_asks(),
    m_bids(),
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
    }

    /// Push the order into the queue first.
    m_rearrangers[order_wrapper->symbol()].push(order_wrapper);

    while (true) {
        /// Loop until no more orders in the queue.
        const auto& next_order_wrapper = m_rearrangers[order_wrapper->symbol()].pop();
        if (!next_order_wrapper.has_value())
            break;

        /// If in call auction stage.
        if (next_order_wrapper.value()->exchange_time() < 925000) {
            m_call_auction_holders[next_order_wrapper.value()->symbol()].push(next_order_wrapper.value());
        }
        /// If in continuous trade stage.
        else {
            auction(next_order_wrapper.value());
        }
    }
}

void trade::booker::Booker::trade(const TradeTickPtr& trade_tick)
{
    /// Only accepts trade made in auction stage.
    if (trade_tick->exchange_time() == 925000) {
        m_call_auction_holders[trade_tick->symbol()].trade(*trade_tick);

        /// Report trade.
        const auto md_trade = std::make_shared<types::MdTrade>();

        md_trade->set_symbol(trade_tick->symbol());
        md_trade->set_price(trade_tick->exec_price());
        md_trade->set_quantity(trade_tick->exec_quantity());

        m_reporter->md_trade_generated(md_trade);

        /// Report market price.
        m_reporter->market_price(trade_tick->symbol(), trade_tick->exec_price());
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

    m_reporter->md_trade_generated(md_trade);
    m_reporter->market_price(book->symbol(), BookerCommonData::to_price(price));
    generate_level_price(book->symbol());
    m_reporter->level_price(m_asks, m_bids);
}

void trade::booker::Booker::generate_level_price(const std::string& symbol)
{
    /// Set asks and bids to 0 first.
    m_asks.fill(0.0);
    m_bids.fill(0.0);

    auto index = 0;
    for (const auto& price : m_books[symbol]->asks() | std::views::keys) {
        if (index > m_asks.size())
            break;
        m_asks[index++] = BookerCommonData::to_price(price.price());
    }

    index = 0;
    for (const auto& price : m_books[symbol]->bids() | std::views::keys) {
        if (index > m_bids.size())
            break;
        m_bids[index++] = BookerCommonData::to_price(price.price());
    }
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
    m_rearrangers.emplace(symbol, Rearranger());
    m_call_auction_holders.emplace(symbol, CallAuctionHolder());

    m_books[symbol]->set_symbol(symbol);
    m_books[symbol]->set_trade_listener(this);

    logger->debug("Created new order book for symbol {}", symbol);
}