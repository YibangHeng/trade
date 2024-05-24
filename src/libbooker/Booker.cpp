#include "libbooker/Booker.h"
#include "libbooker/BookerCommonData.h"

trade::broker::Booker::Booker(
    const std::vector<std::string>& symbols,
    const std::shared_ptr<reporter::IReporter>& reporter
) : AppBase("Booker"),
    m_reporter(reporter)
{
    for (const auto& symbol : symbols) {
        books.emplace(symbol, std::make_shared<liquibook::book::OrderBook<OrderWrapperPtr>>(symbol));
        books.at(symbol)->set_trade_listener(this);

        logger->debug("Created new order book for symbol {}", symbol);
    }
}

void trade::broker::Booker::add(const std::shared_ptr<types::OrderTick>& order_tick)
{
    if (!books.contains(order_tick->symbol())) {
        books.emplace(order_tick->symbol(), std::make_shared<liquibook::book::OrderBook<OrderWrapperPtr>>());
        books[order_tick->symbol()]->set_symbol(order_tick->symbol());
        books[order_tick->symbol()]->set_trade_listener(this);

        logger->debug("Created new order book for symbol {}", order_tick->symbol());
    }

    books[order_tick->symbol()]->add(std::make_shared<OrderWrapper>(order_tick));
}

void trade::broker::Booker::cancel(const std::shared_ptr<types::OrderTick>& order_tick)
{
    if (!books.contains(order_tick->symbol())) {
        books.emplace(order_tick->symbol(), std::make_shared<liquibook::book::OrderBook<OrderWrapperPtr>>());
        books[order_tick->symbol()]->set_symbol(order_tick->symbol());
        books[order_tick->symbol()]->set_trade_listener(this);
    }

    books[order_tick->symbol()]->cancel(std::make_shared<OrderWrapper>(order_tick));
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
