#include "libbooker/Booker.h"
#include "libbooker/BookerCommonData.h"
#include "utilities/ToJSON.hpp"

trade::broker::Booker::Booker(
    const std::vector<std::string>& symbols,
    const std::shared_ptr<reporter::IReporter>& reporter
) : AppBase("Booker"),
    asks(),
    bids(),
    m_reporter(reporter)
{
    for (const auto& symbol : symbols)
        new_booker(symbol);
}

void trade::broker::Booker::add(const std::shared_ptr<types::OrderTick>& order_tick)
{
    new_booker(order_tick->symbol());

    std::shared_ptr<OrderWrapper> order_wrapper;

    /// Check if order already exists.
    if (orders.contains(order_tick->unique_id())) {
        order_wrapper = orders[order_tick->unique_id()];
    }
    else {
        order_wrapper = std::make_shared<OrderWrapper>(order_tick);
        orders.emplace(order_tick->unique_id(), order_wrapper);
    }

    switch (order_tick->order_type()) {
    case types::OrderType::limit:
    case types::OrderType::market: {
        books[order_tick->symbol()]->add(order_wrapper);
        break;
    }
        /// TODO: Make the matching logic for best price clearer.
    case types::OrderType::best_price: {
        double best_price;

        if (order_wrapper->is_buy()) {
            const auto bids = books[order_tick->symbol()]->bids();

            if (bids.empty()) {
                logger->error("A best price order {} arrived while no bid exists", order_wrapper->unique_id());
                break;
            }

            best_price = BookerCommonData::to_price(bids.begin()->first.price());
        }
        else {
            const auto asks = books[order_tick->symbol()]->asks();

            if (asks.empty()) {
                logger->error("A best price order {} arrived while no ask exists", order_wrapper->unique_id());
                break;
            }

            best_price = BookerCommonData::to_price(asks.begin()->first.price());
        }

        order_wrapper->to_limit_order(best_price);

        books[order_tick->symbol()]->add(order_wrapper);
        break;
    }
    case types::OrderType::cancel: {
        books[order_tick->symbol()]->cancel(order_wrapper);
        break;
    }
    default: {
        logger->error("Unsupported order type: {}", utilities::ToJSON()(*order_tick));
    }
    }
}

void trade::broker::Booker::on_trade(
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
    m_reporter->level_price(asks, bids);
}

void trade::broker::Booker::generate_level_price(const std::string& symbol)
{
    /// Set asks and bids to 0 first.
    asks.fill(0.0);
    bids.fill(0.0);

    auto index = 0;
    for (const auto& price : books[symbol]->asks() | std::views::keys) {
        if (index > asks.size())
            break;
        asks[index++] = BookerCommonData::to_price(price.price());
    }

    index = 0;
    for (const auto& price : books[symbol]->bids() | std::views::keys) {
        if (index > bids.size())
            break;
        bids[index++] = BookerCommonData::to_price(price.price());
    }
}

void trade::broker::Booker::on_reject(const std::shared_ptr<OrderWrapper>& order, const char* reason)
{
    logger->error("Order {} was rejected: {}", order->unique_id(), reason);
}

void trade::broker::Booker::on_cancel_reject(const std::shared_ptr<OrderWrapper>& order, const char* reason)
{
    logger->error("Cancel for {} was rejected: {}", order->unique_id(), reason);
}

void trade::broker::Booker::on_replace_reject(const std::shared_ptr<OrderWrapper>& order, const char* reason)
{
    logger->error("Replace for {} was rejected: {}", order->unique_id(), reason);
}

void trade::broker::Booker::new_booker(const std::string& symbol)
{
    /// Do nothing if the order book already exists.
    if (books.contains(symbol))
        return;

    books.emplace(symbol, std::make_shared<liquibook::book::OrderBook<OrderWrapperPtr>>(symbol));

    books[symbol]->set_symbol(symbol);
    books[symbol]->set_trade_listener(this);

    logger->debug("Created new order book for symbol {}", symbol);
}