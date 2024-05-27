#include "libbooker/Booker.h"
#include "libbooker/BookerCommonData.h"
#include "utilities/ToJSON.hpp"

trade::broker::Booker::Booker(
    const std::vector<std::string>& symbols,
    const std::shared_ptr<reporter::IReporter>& reporter
) : AppBase("Booker"),
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
    case types::OrderType::market:
    case types::OrderType::best_price: {
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