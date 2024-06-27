#include "libbooker/Booker.h"
#include "libbooker/BookerCommonData.h"
#include "utilities/TimeHelper.hpp"
#include "utilities/ToJSON.hpp"

trade::booker::Booker::Booker(
    const std::vector<std::string>& symbols,
    const std::shared_ptr<reporter::IReporter>& reporter,
    const bool enable_validation
) : AppBase("Booker"),
    m_in_continuous_stage(),
    m_md_validator(enable_validation ? MdValidator() : std::optional<MdValidator> {}),
    m_reporter(reporter)
{
    for (const auto& symbol : symbols)
        new_booker(symbol);

    if (m_md_validator.has_value())
        logger->info("Real-time market data validation enabled");
    else
        logger->info("Real-time market data validation disabled");
}

void trade::booker::Booker::add(const OrderTickPtr& order_tick)
{
    logger->debug("Fed order: {}", utilities::ToJSON()(*order_tick));

    new_booker(order_tick->symbol());

    OrderWrapperPtr order_wrapper;

    /// Check if order already exists.
    if (m_orders.contains(order_tick->unique_id())) {
        order_wrapper = m_orders[order_tick->unique_id()];

        if (order_tick->order_type() != types::OrderType::cancel) {
            logger->warn("Received duplicated order with unexpected order_type: the existing order is {} and the new arrived order is {}", utilities::ToJSON()(*order_wrapper->raw_order_tick()), utilities::ToJSON()(*order_tick));
            return;
        }

        /// Changing order status manually is needed since booker does not use order_tick.
        order_wrapper->mark_as_cancel();
    }
    else {
        order_wrapper = std::make_shared<OrderWrapper>(order_tick);
        m_orders.emplace(order_tick->unique_id(), order_wrapper);
    }

    /// If in call auction stage.
    if (order_wrapper->exchange_time() < 92500000) {
        m_call_auction_holders[order_wrapper->symbol()].push(order_wrapper);
    }
    /// If in continuous trade stage.
    else {
        auction(order_wrapper);
    }
}

void trade::booker::Booker::trade(const TradeTickPtr& trade_tick)
{
    /// If trade made in call auction stage.
    if (trade_tick->exchange_time() / 1000 == 92500) [[unlikely]] {
        m_call_auction_holders[trade_tick->symbol()].trade(*trade_tick);

        /// Report trade.
        const auto l2_tick = std::make_shared<types::L2Tick>();

        l2_tick->set_symbol(trade_tick->symbol());
        l2_tick->set_price(trade_tick->exec_price());
        l2_tick->set_quantity(trade_tick->exec_quantity());
        l2_tick->set_ask_unique_id(trade_tick->ask_unique_id());
        l2_tick->set_bid_unique_id(trade_tick->bid_unique_id());

        m_reporter->l2_tick_generated(l2_tick);

        return;
    }

    if (m_md_validator.has_value() && !m_md_validator.value().check(trade_tick))
        logger->error("Verification failed for {}'s trade tick", trade_tick->symbol());
}

void trade::booker::Booker::l2(const L2TickPtr& l2_tick) const
{
    if (m_md_validator.has_value() && !m_md_validator.value().check(l2_tick))
        logger->error("Verification failed for {}'s l2 snapshot", l2_tick->symbol());
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

            logger->debug("Added unfinished order in call auction stage: {}", utilities::ToJSON()(*order_tick));

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
    /// Booker::on_fill() is called before Booker::on_trade().

    m_latest_l2_tick->set_symbol(book->symbol());
    m_latest_l2_tick->set_price(BookerCommonData::to_price(price));
    m_latest_l2_tick->set_quantity(BookerCommonData::to_quantity(qty));

    generate_level_price();

    m_md_validator.has_value() ? m_md_validator.value().l2_tick_generated(m_latest_l2_tick) : void(); /// Feed to validator first.
    m_reporter->l2_tick_generated(m_latest_l2_tick);
}

void trade::booker::Booker::on_reject(const OrderWrapperPtr& order, const char* reason)
{
    logger->error("Order {} was rejected: {}", order->unique_id(), reason);
}

void trade::booker::Booker::on_fill(
    const OrderWrapperPtr& order,
    const OrderWrapperPtr& matched_order,
    liquibook::book::Quantity fill_qty,
    liquibook::book::Price fill_price
)
{
    m_latest_l2_tick = std::make_shared<types::L2Tick>();

    if (order->is_buy()) {
        m_latest_l2_tick->set_ask_unique_id(matched_order->unique_id());
        m_latest_l2_tick->set_bid_unique_id(order->unique_id());
    }
    else {
        m_latest_l2_tick->set_ask_unique_id(order->unique_id());
        m_latest_l2_tick->set_bid_unique_id(matched_order->unique_id());
    }
}

void trade::booker::Booker::on_cancel_reject(const OrderWrapperPtr& order, const char* reason)
{
    logger->error("Cancel for {} was rejected: {}", order->unique_id(), reason);
}

void trade::booker::Booker::on_replace_reject(const OrderWrapperPtr& order, const char* reason)
{
    logger->error("Replace for {} was rejected: {}", order->unique_id(), reason);
}

void trade::booker::Booker::generate_level_price()
{
    auto ask_it = m_books[m_latest_l2_tick->symbol()]->asks().begin();
    auto bid_it = m_books[m_latest_l2_tick->symbol()]->bids().begin();

    if (ask_it != m_books[m_latest_l2_tick->symbol()]->asks().end()) m_latest_l2_tick->set_sell_price_1(BookerCommonData::to_price(ask_it++->first.price()));
    if (ask_it != m_books[m_latest_l2_tick->symbol()]->asks().end()) m_latest_l2_tick->set_sell_quantity_1(BookerCommonData::to_quantity(ask_it->second.open_qty()));
    if (ask_it != m_books[m_latest_l2_tick->symbol()]->asks().end()) m_latest_l2_tick->set_sell_price_2(BookerCommonData::to_price(ask_it++->first.price()));
    if (ask_it != m_books[m_latest_l2_tick->symbol()]->asks().end()) m_latest_l2_tick->set_sell_quantity_2(BookerCommonData::to_quantity(ask_it->second.open_qty()));
    if (ask_it != m_books[m_latest_l2_tick->symbol()]->asks().end()) m_latest_l2_tick->set_sell_price_3(BookerCommonData::to_price(ask_it++->first.price()));
    if (ask_it != m_books[m_latest_l2_tick->symbol()]->asks().end()) m_latest_l2_tick->set_sell_quantity_3(BookerCommonData::to_quantity(ask_it->second.open_qty()));
    if (ask_it != m_books[m_latest_l2_tick->symbol()]->asks().end()) m_latest_l2_tick->set_sell_price_4(BookerCommonData::to_price(ask_it++->first.price()));
    if (ask_it != m_books[m_latest_l2_tick->symbol()]->asks().end()) m_latest_l2_tick->set_sell_quantity_4(BookerCommonData::to_quantity(ask_it->second.open_qty()));
    if (ask_it != m_books[m_latest_l2_tick->symbol()]->asks().end()) m_latest_l2_tick->set_sell_price_5(BookerCommonData::to_price(ask_it++->first.price()));
    if (ask_it != m_books[m_latest_l2_tick->symbol()]->asks().end()) m_latest_l2_tick->set_sell_quantity_5(BookerCommonData::to_quantity(ask_it->second.open_qty()));
    if (ask_it != m_books[m_latest_l2_tick->symbol()]->asks().end()) m_latest_l2_tick->set_sell_price_6(BookerCommonData::to_price(ask_it++->first.price()));
    if (ask_it != m_books[m_latest_l2_tick->symbol()]->asks().end()) m_latest_l2_tick->set_sell_quantity_6(BookerCommonData::to_quantity(ask_it->second.open_qty()));
    if (ask_it != m_books[m_latest_l2_tick->symbol()]->asks().end()) m_latest_l2_tick->set_sell_price_7(BookerCommonData::to_price(ask_it++->first.price()));
    if (ask_it != m_books[m_latest_l2_tick->symbol()]->asks().end()) m_latest_l2_tick->set_sell_quantity_7(BookerCommonData::to_quantity(ask_it->second.open_qty()));
    if (ask_it != m_books[m_latest_l2_tick->symbol()]->asks().end()) m_latest_l2_tick->set_sell_price_8(BookerCommonData::to_price(ask_it++->first.price()));
    if (ask_it != m_books[m_latest_l2_tick->symbol()]->asks().end()) m_latest_l2_tick->set_sell_quantity_8(BookerCommonData::to_quantity(ask_it->second.open_qty()));
    if (ask_it != m_books[m_latest_l2_tick->symbol()]->asks().end()) m_latest_l2_tick->set_sell_price_9(BookerCommonData::to_price(ask_it++->first.price()));
    if (ask_it != m_books[m_latest_l2_tick->symbol()]->asks().end()) m_latest_l2_tick->set_sell_quantity_9(BookerCommonData::to_quantity(ask_it->second.open_qty()));
    if (ask_it != m_books[m_latest_l2_tick->symbol()]->asks().end()) m_latest_l2_tick->set_sell_price_10(BookerCommonData::to_price(ask_it++->first.price()));
    if (ask_it != m_books[m_latest_l2_tick->symbol()]->asks().end()) m_latest_l2_tick->set_sell_quantity_10(BookerCommonData::to_quantity(ask_it->second.open_qty()));

    if (bid_it != m_books[m_latest_l2_tick->symbol()]->bids().end()) m_latest_l2_tick->set_buy_price_1(BookerCommonData::to_price(bid_it++->first.price()));
    if (bid_it != m_books[m_latest_l2_tick->symbol()]->bids().end()) m_latest_l2_tick->set_buy_quantity_1(BookerCommonData::to_quantity(bid_it->second.open_qty()));
    if (bid_it != m_books[m_latest_l2_tick->symbol()]->bids().end()) m_latest_l2_tick->set_buy_price_2(BookerCommonData::to_price(bid_it++->first.price()));
    if (bid_it != m_books[m_latest_l2_tick->symbol()]->bids().end()) m_latest_l2_tick->set_buy_quantity_2(BookerCommonData::to_quantity(bid_it->second.open_qty()));
    if (bid_it != m_books[m_latest_l2_tick->symbol()]->bids().end()) m_latest_l2_tick->set_buy_price_3(BookerCommonData::to_price(bid_it++->first.price()));
    if (bid_it != m_books[m_latest_l2_tick->symbol()]->bids().end()) m_latest_l2_tick->set_buy_quantity_3(BookerCommonData::to_quantity(bid_it->second.open_qty()));
    if (bid_it != m_books[m_latest_l2_tick->symbol()]->bids().end()) m_latest_l2_tick->set_buy_price_4(BookerCommonData::to_price(bid_it++->first.price()));
    if (bid_it != m_books[m_latest_l2_tick->symbol()]->bids().end()) m_latest_l2_tick->set_buy_quantity_4(BookerCommonData::to_quantity(bid_it->second.open_qty()));
    if (bid_it != m_books[m_latest_l2_tick->symbol()]->bids().end()) m_latest_l2_tick->set_buy_price_5(BookerCommonData::to_price(bid_it++->first.price()));
    if (bid_it != m_books[m_latest_l2_tick->symbol()]->bids().end()) m_latest_l2_tick->set_buy_quantity_5(BookerCommonData::to_quantity(bid_it->second.open_qty()));
    if (bid_it != m_books[m_latest_l2_tick->symbol()]->bids().end()) m_latest_l2_tick->set_buy_price_6(BookerCommonData::to_price(bid_it++->first.price()));
    if (bid_it != m_books[m_latest_l2_tick->symbol()]->bids().end()) m_latest_l2_tick->set_buy_quantity_6(BookerCommonData::to_quantity(bid_it->second.open_qty()));
    if (bid_it != m_books[m_latest_l2_tick->symbol()]->bids().end()) m_latest_l2_tick->set_buy_price_7(BookerCommonData::to_price(bid_it++->first.price()));
    if (bid_it != m_books[m_latest_l2_tick->symbol()]->bids().end()) m_latest_l2_tick->set_buy_quantity_7(BookerCommonData::to_quantity(bid_it->second.open_qty()));
    if (bid_it != m_books[m_latest_l2_tick->symbol()]->bids().end()) m_latest_l2_tick->set_buy_price_8(BookerCommonData::to_price(bid_it++->first.price()));
    if (bid_it != m_books[m_latest_l2_tick->symbol()]->bids().end()) m_latest_l2_tick->set_buy_quantity_8(BookerCommonData::to_quantity(bid_it->second.open_qty()));
    if (bid_it != m_books[m_latest_l2_tick->symbol()]->bids().end()) m_latest_l2_tick->set_buy_price_9(BookerCommonData::to_price(bid_it++->first.price()));
    if (bid_it != m_books[m_latest_l2_tick->symbol()]->bids().end()) m_latest_l2_tick->set_buy_quantity_9(BookerCommonData::to_quantity(bid_it->second.open_qty()));
    if (bid_it != m_books[m_latest_l2_tick->symbol()]->bids().end()) m_latest_l2_tick->set_buy_price_10(BookerCommonData::to_price(bid_it++->first.price()));
    if (bid_it != m_books[m_latest_l2_tick->symbol()]->bids().end()) m_latest_l2_tick->set_buy_quantity_10(BookerCommonData::to_quantity(bid_it->second.open_qty()));
}

void trade::booker::Booker::new_booker(const std::string& symbol)
{
    /// Do nothing if the order book already exists.
    if (m_books.contains(symbol)) [[likely]]
        return;

    m_books.emplace(symbol, std::make_shared<liquibook::book::OrderBook<OrderWrapperPtr>>(symbol));
    m_call_auction_holders.emplace(symbol, CallAuctionHolder());

    m_books[symbol]->set_symbol(symbol);
    m_books[symbol]->set_order_listener(this);
    m_books[symbol]->set_trade_listener(this);

    logger->info("Created new order book for symbol {}", symbol);
}
