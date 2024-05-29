#include "libbooker/Booker.h"
#include "libbooker/BookerCommonData.h"
#include "utilities/ToJSON.hpp"

trade::booker::Booker::Booker(
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

void trade::booker::Booker::add(const OrderTickPtr& order_tick)
{
    new_booker(order_tick->symbol());

    OrderWrapperPtr order_wrapper;

    /// Check if order already exists.
    if (orders.contains(order_tick->unique_id())) {
        order_wrapper = orders[order_tick->unique_id()];
    }
    else {
        order_wrapper = std::make_shared<OrderWrapper>(order_tick);
        orders.emplace(order_tick->unique_id(), order_wrapper);
    }

    /// Push the order into the queue first.
    rearrangers[order_wrapper->symbol()].push(order_wrapper);

    while (true) {
        /// Loop until no more orders in the queue.
        const auto& next_order_wrapper = rearrangers[order_wrapper->symbol()].pop();
        if (!next_order_wrapper.has_value())
            break;

        switch (next_order_wrapper.value()->order_type()) {
        case types::OrderType::limit:
        case types::OrderType::market: {
            books[order_tick->symbol()]->add(next_order_wrapper.value());
            break;
        }
            /// TODO: Make the matching logic for best price clearer.
        case types::OrderType::best_price: {
            double best_price;

            if (next_order_wrapper.value()->is_buy()) {
                const auto bids = books[order_tick->symbol()]->bids();

                if (bids.empty()) {
                    logger->error("A best price order {} arrived while no bid exists", next_order_wrapper.value()->unique_id());
                    break;
                }

                best_price = BookerCommonData::to_price(bids.begin()->first.price());
            }
            else {
                const auto asks = books[order_tick->symbol()]->asks();

                if (asks.empty()) {
                    logger->error("A best price order {} arrived while no ask exists", next_order_wrapper.value()->unique_id());
                    break;
                }

                best_price = BookerCommonData::to_price(asks.begin()->first.price());
            }

            next_order_wrapper.value()->to_limit_order(best_price);

            books[order_tick->symbol()]->add(next_order_wrapper.value());
            break;
        }
        case types::OrderType::cancel: {
            books[order_tick->symbol()]->cancel(next_order_wrapper.value());
            break;
        }
        default: {
            logger->error("Unsupported order type: {}", utilities::ToJSON()(*order_tick));
        }
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
    m_reporter->level_price(asks, bids);
}

void trade::booker::Booker::generate_level_price(const std::string& symbol)
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
    if (books.contains(symbol))
        return;

    books.emplace(symbol, std::make_shared<liquibook::book::OrderBook<OrderWrapperPtr>>(symbol));
    rearrangers.emplace(symbol, Rearranger());

    books[symbol]->set_symbol(symbol);
    books[symbol]->set_trade_listener(this);

    logger->debug("Created new order book for symbol {}", symbol);
}