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
        order_wrapper->mark_as_cancel(order_tick->exchange_time());

        auction(order_wrapper);
    }
    else {
        order_wrapper = std::make_shared<OrderWrapper>(order_tick);

        /// If in call auction stage.
        if (order_wrapper->exchange_time() < 92500000) {
            m_call_auction_holders[order_wrapper->symbol()].push(order_wrapper);
        }
        /// If in continuous trade stage.
        else {
            /// If order arrived while a remained market order exists.
            if (m_market_order.contains(order_tick->symbol()) && m_market_order[order_tick->symbol()]->unique_id() != order_tick->unique_id()) {
                const auto order_wrapper_for_remained_market_order = std::make_shared<OrderWrapper>(m_market_order[order_tick->symbol()]);
                order_wrapper_for_remained_market_order->to_limit_order(m_market_order[order_tick->symbol()]->price_1000x());

                auction(order_wrapper_for_remained_market_order);

                m_market_order.erase(order_tick->symbol());
            }

            /// If order is a market order or not.
            if (order_tick->order_type() == types::OrderType::market) {
                m_market_order[order_tick->symbol()] = order_tick;
            }
            else {
                m_orders.emplace(order_tick->unique_id(), order_wrapper);

                auction(order_wrapper);
            }
        }
    }
}

bool trade::booker::Booker::trade(const TradeTickPtr& trade_tick)
{
    /// If trade arrived while a remained market order exists (for SZSE).
    if (m_market_order.contains(trade_tick->symbol())) {
        /// Create a virtual limit order for this market order.
        const auto order_tick = create_virtual_szse_order_tick(trade_tick);

        if (order_tick == nullptr)
            return false;

        logger->debug("Created virtual limit order {} for trade tick: {}", utilities::ToJSON()(*order_tick), utilities::ToJSON()(*trade_tick));

        add(order_tick);

        const int64_t original_quantity = m_market_order[trade_tick->symbol()]->quantity();
        m_market_order[trade_tick->symbol()]->set_price_1000x(trade_tick->exec_price_1000x());
        m_market_order[trade_tick->symbol()]->set_quantity(original_quantity - trade_tick->exec_quantity());
        if (m_market_order[trade_tick->symbol()]->quantity() == 0) {
            m_market_order.erase(trade_tick->symbol());
        }

        return true;
    }

    const auto exchange_time = trade_tick->exchange_time() / 1000;

    /// If trade made in open call auction stage.
    if (exchange_time >= 92500 && exchange_time < 93000)
        m_call_auction_holders[trade_tick->symbol()].trade(*trade_tick);

    /// If trade made in open/close continuous stage.
    if ((exchange_time >= 92500 && exchange_time < 93000)
        || (exchange_time >= 145700 && exchange_time <= 151000)) [[unlikely]] {
        /// Report trade.
        const auto generated_l2_tick = std::make_shared<types::GeneratedL2Tick>();

        generated_l2_tick->set_symbol(trade_tick->symbol());
        generated_l2_tick->set_price_1000x(trade_tick->exec_price_1000x());
        generated_l2_tick->set_quantity(trade_tick->exec_quantity());
        generated_l2_tick->set_ask_unique_id(trade_tick->ask_unique_id());
        generated_l2_tick->set_bid_unique_id(trade_tick->bid_unique_id());
        generated_l2_tick->set_exchange_time(trade_tick->exchange_time());

        m_reporter->l2_tick_generated(generated_l2_tick);

        return true;
    }

    /// If trade arrived while no order for this trade exists (for SSE).
    if (!m_orders.contains(trade_tick->ask_unique_id())) {
        /// Create a virtual limit order for this trade.
        const auto order_tick = create_virtual_sse_order_tick(trade_tick, types::SideType::sell);

        if (order_tick == nullptr)
            return false;

        logger->debug("Created virtual limit order {} for trade tick: {}", utilities::ToJSON()(*order_tick), utilities::ToJSON()(*trade_tick));

        add(order_tick);

        m_orders.erase(trade_tick->ask_unique_id());
    }

    /// If trade arrived while no order for this trade exists (for SSE).
    if (!m_orders.contains(trade_tick->bid_unique_id())) {
        /// Create a virtual limit order for this trade.
        const auto order_tick = create_virtual_sse_order_tick(trade_tick, types::SideType::buy);

        if (order_tick == nullptr)
            return false;

        logger->debug("Created virtual limit order {} for trade tick: {}", utilities::ToJSON()(*order_tick), utilities::ToJSON()(*trade_tick));

        add(order_tick);

        m_orders.erase(trade_tick->bid_unique_id());
    }

    if (m_md_validator.has_value() && !m_md_validator.value().check(trade_tick)) {
        if (!m_failed_symbols.contains(trade_tick->symbol())) {
            logger->error("Verification failed for trade tick: {}", utilities::ToJSON()(*trade_tick));
            m_failed_symbols.insert(trade_tick->symbol());
        }
        return false;
    }

    return true;
}

void trade::booker::Booker::switch_to_continuous_stage()
{
    if (m_in_continuous_stage) [[likely]]
        return;

    logger->info("Switching to continuous trade stage at {}", utilities::Now<std::string>()());

    /// Move all unfinished orders from call auction stage to continuous trade stage.
    for (auto& call_auction_holder : m_call_auction_holders | std::views::values) {
        while (true) {
            const auto order_tick = call_auction_holder.pop();
            if (order_tick == nullptr)
                break;

            logger->debug("Added unfinished order in call auction stage: {}", utilities::ToJSON()(*order_tick));

            order_tick->set_exchange_time(93000000);

            add(order_tick);
        }
    }

    m_in_continuous_stage = true;

    logger->info("Switched to continuous trade stage at {}", utilities::Now<std::string>()());
}

void trade::booker::Booker::auction(const OrderWrapperPtr& order_wrapper)
{
    switch (order_wrapper->order_type()) {
    case types::OrderType::limit: {
        m_books[order_wrapper->symbol()]->add(order_wrapper);
        break;
    }
    case types::OrderType::best_price: {
        int64_t price_1000x;

        if (order_wrapper->is_buy()) {
            const auto bids = m_books[order_wrapper->symbol()]->bids();

            if (bids.empty())
                break;

            price_1000x = BookerCommonData::to_price(bids.begin()->first.price());
        }
        else {
            const auto asks = m_books[order_wrapper->symbol()]->asks();

            if (asks.empty())
                break;

            price_1000x = BookerCommonData::to_price(asks.begin()->first.price());
        }

        order_wrapper->to_limit_order(price_1000x);

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
    m_latest_l2_tick->set_price_1000x(BookerCommonData::to_price(price));
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
    m_latest_l2_tick = std::make_shared<types::GeneratedL2Tick>();

    if (order->is_buy()) {
        m_latest_l2_tick->set_ask_unique_id(matched_order->unique_id());
        m_latest_l2_tick->set_bid_unique_id(order->unique_id());
    }
    else {
        m_latest_l2_tick->set_ask_unique_id(order->unique_id());
        m_latest_l2_tick->set_bid_unique_id(matched_order->unique_id());
    }

    m_latest_l2_tick->set_exchange_time(order->exchange_time());
}

void trade::booker::Booker::on_cancel_reject(const OrderWrapperPtr& order, const char* reason)
{
    logger->error("Cancel for {} was rejected: {}", order->unique_id(), reason);
}

void trade::booker::Booker::on_replace_reject(const OrderWrapperPtr& order, const char* reason)
{
    logger->error("Replace for {} was rejected: {}", order->unique_id(), reason);
}

trade::booker::OrderTickPtr trade::booker::Booker::create_virtual_sse_order_tick(const TradeTickPtr& trade_tick, const types::SideType side)
{
    auto order_tick = std::make_shared<types::OrderTick>();

    switch (side) {
    case types::SideType::buy: {
        order_tick->set_unique_id(trade_tick->bid_unique_id());
        break;
    }
    case types::SideType::sell: {
        order_tick->set_unique_id(trade_tick->ask_unique_id());
        break;
    }
    default: {
        return nullptr;
    }
    }

    order_tick->set_order_type(types::OrderType::limit);
    order_tick->set_symbol(trade_tick->symbol());
    order_tick->set_side(side);
    order_tick->set_price_1000x(trade_tick->exec_price_1000x());
    order_tick->set_quantity(trade_tick->exec_quantity());
    order_tick->set_exchange_time(trade_tick->exchange_time());

    return order_tick;
}

trade::booker::OrderTickPtr trade::booker::Booker::create_virtual_szse_order_tick(const TradeTickPtr& trade_tick)
{
    if (!m_market_order.contains(trade_tick->symbol()))
        return nullptr;

    auto order_tick = std::make_shared<types::OrderTick>();

    order_tick->set_unique_id(m_market_order[trade_tick->symbol()]->unique_id());
    order_tick->set_order_type(types::OrderType::limit);
    order_tick->set_symbol(trade_tick->symbol());
    order_tick->set_side(m_market_order[trade_tick->symbol()]->side());
    order_tick->set_price_1000x(trade_tick->exec_price_1000x());
    order_tick->set_quantity(trade_tick->exec_quantity());
    order_tick->set_exchange_time(m_market_order[trade_tick->symbol()]->exchange_time());

    /// For avoiding issus of deplicated order.
    m_orders.erase(m_market_order[trade_tick->symbol()]->unique_id());

    return order_tick;
}

void trade::booker::Booker::generate_level_price()
{
    auto ask_it = m_books[m_latest_l2_tick->symbol()]->asks().begin();

    std::array<int64_t, 5> ask_price_level {};
    std::array<int64_t, 5> ask_quantity_level {};
    auto ask_price_level_it    = ask_price_level.begin();
    auto ask_quantity_level_it = ask_quantity_level.begin();

    while (ask_it != m_books[m_latest_l2_tick->symbol()]->asks().end()) {
        if (*ask_price_level_it == 0)
            *ask_price_level_it = BookerCommonData::to_price(ask_it->first.price());

        if (*ask_price_level_it != BookerCommonData::to_price(ask_it->first.price())) {
            ask_price_level_it++;
            ask_quantity_level_it++;

            if (ask_price_level_it == ask_price_level.end() || ask_quantity_level_it == ask_quantity_level.end())
                break;
        }

        *ask_price_level_it = BookerCommonData::to_price(ask_it->first.price());
        *ask_quantity_level_it += BookerCommonData::to_quantity(ask_it->second.open_qty());

        ask_it++;
    }

    m_latest_l2_tick->set_sell_price_1000x_1(ask_price_level[0]);
    m_latest_l2_tick->set_sell_price_1000x_2(ask_price_level[1]);
    m_latest_l2_tick->set_sell_price_1000x_3(ask_price_level[2]);
    m_latest_l2_tick->set_sell_price_1000x_4(ask_price_level[3]);
    m_latest_l2_tick->set_sell_price_1000x_5(ask_price_level[4]);
    m_latest_l2_tick->set_sell_quantity_1(ask_quantity_level[0]);
    m_latest_l2_tick->set_sell_quantity_2(ask_quantity_level[1]);
    m_latest_l2_tick->set_sell_quantity_3(ask_quantity_level[2]);
    m_latest_l2_tick->set_sell_quantity_4(ask_quantity_level[3]);
    m_latest_l2_tick->set_sell_quantity_5(ask_quantity_level[4]);

    auto bid_it = m_books[m_latest_l2_tick->symbol()]->bids().begin();

    std::array<int64_t, 5> bid_price_level {};
    std::array<int64_t, 5> bid_quantity_level {};
    auto bid_price_level_it    = bid_price_level.begin();
    auto bid_quantity_level_it = bid_quantity_level.begin();

    while (bid_it != m_books[m_latest_l2_tick->symbol()]->bids().end()) {
        if (*bid_price_level_it == 0)
            *bid_price_level_it = BookerCommonData::to_price(bid_it->first.price());

        if (*bid_price_level_it != BookerCommonData::to_price(bid_it->first.price())) {
            bid_price_level_it++;
            bid_quantity_level_it++;

            if (bid_price_level_it == bid_price_level.end() || bid_quantity_level_it == bid_quantity_level.end())
                break;
        }

        *bid_price_level_it = BookerCommonData::to_price(bid_it->first.price());
        *bid_quantity_level_it += BookerCommonData::to_quantity(bid_it->second.open_qty());

        bid_it++;
    }

    m_latest_l2_tick->set_buy_price_1000x_1(bid_price_level[0]);
    m_latest_l2_tick->set_buy_price_1000x_2(bid_price_level[1]);
    m_latest_l2_tick->set_buy_price_1000x_3(bid_price_level[2]);
    m_latest_l2_tick->set_buy_price_1000x_4(bid_price_level[3]);
    m_latest_l2_tick->set_buy_price_1000x_5(bid_price_level[4]);
    m_latest_l2_tick->set_buy_quantity_1(bid_quantity_level[0]);
    m_latest_l2_tick->set_buy_quantity_2(bid_quantity_level[1]);
    m_latest_l2_tick->set_buy_quantity_3(bid_quantity_level[2]);
    m_latest_l2_tick->set_buy_quantity_4(bid_quantity_level[3]);
    m_latest_l2_tick->set_buy_quantity_5(bid_quantity_level[4]);
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
