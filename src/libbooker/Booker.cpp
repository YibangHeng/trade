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

        generated_l2_tick->set_result(true); /// TODO: Use eunms to identify trades that made in call auction stage.

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

    m_latest_l2_tick->set_result(!m_failed_symbols.contains(book->symbol()));

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
    const liquibook::book::Quantity fill_qty,
    const liquibook::book::Price fill_price
)
{
    generate_level_price(order->symbol());
    generate_statistic_data(order->symbol(), order, matched_order, fill_qty, fill_price);
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

void trade::booker::Booker::generate_level_price(const std::string& symbol)
{
    m_latest_l2_tick = std::make_shared<types::GeneratedL2Tick>();

    m_latest_l2_tick->set_symbol(symbol);

    /// Record previous price 1.
    m_previous_price_1000x_1[symbol].buy  = m_ask_price_levels[symbol][0];
    m_previous_price_1000x_1[symbol].sell = m_bid_price_levels[symbol][0];

    /// Clear m_ask/bid_price/quantity_levels.
    m_ask_price_levels[symbol].fill(0);
    m_bid_price_levels[symbol].fill(0);
    m_ask_quantity_levels[symbol].fill(0);
    m_bid_quantity_levels[symbol].fill(0);

    /// Ask booker.
    auto ask_it = m_books[symbol]->asks().begin();

    /// Ask price/quantity levels.
    auto ask_price_level_it    = m_ask_price_levels[symbol].begin();
    auto ask_quantity_level_it = m_ask_quantity_levels[symbol].begin();

    while (ask_it != m_books[symbol]->asks().end()) {
        if (*ask_price_level_it == 0)
            *ask_price_level_it = BookerCommonData::to_price(ask_it->first.price());

        if (*ask_price_level_it != BookerCommonData::to_price(ask_it->first.price())) {
            ask_price_level_it++;
            ask_quantity_level_it++;

            if (ask_price_level_it == m_ask_price_levels[symbol].end()
                || ask_quantity_level_it == m_ask_quantity_levels[symbol].end())
                break;
        }

        *ask_price_level_it = BookerCommonData::to_price(ask_it->first.price());
        *ask_quantity_level_it += BookerCommonData::to_quantity(ask_it->second.open_qty());

        ask_it++;
    }

    /// Bid booker.
    auto bid_it = m_books[symbol]->bids().begin();

    /// Bid price/quantity levels.
    auto bid_price_level_it    = m_bid_price_levels[symbol].begin();
    auto bid_quantity_level_it = m_bid_quantity_levels[symbol].begin();

    while (bid_it != m_books[symbol]->bids().end()) {
        if (*bid_price_level_it == 0)
            *bid_price_level_it = BookerCommonData::to_price(bid_it->first.price());

        if (*bid_price_level_it != BookerCommonData::to_price(bid_it->first.price())) {
            bid_price_level_it++;
            bid_quantity_level_it++;

            if (bid_price_level_it == m_bid_price_levels[symbol].end()
                || bid_quantity_level_it == m_bid_quantity_levels[symbol].end())
                break;
        }

        *bid_price_level_it = BookerCommonData::to_price(bid_it->first.price());
        *bid_quantity_level_it += BookerCommonData::to_quantity(bid_it->second.open_qty());

        bid_it++;
    }

    m_ask_queue_size[symbol] = static_cast<int64_t>(m_books[symbol]->asks().size());
    m_bid_queue_size[symbol] = static_cast<int64_t>(m_books[symbol]->bids().size());
}

void trade::booker::Booker::generate_statistic_data(
    const std::string& symbol,
    const OrderWrapperPtr& order,
    const OrderWrapperPtr& matched_order,
    const liquibook::book::Quantity fill_qty,
    const liquibook::book::Price fill_price
)
{
    /// L2 data.
    m_latest_l2_tick->set_sell_price_1000x_1(m_ask_price_levels[symbol][0]);
    m_latest_l2_tick->set_sell_price_1000x_2(m_ask_price_levels[symbol][1]);
    m_latest_l2_tick->set_sell_price_1000x_3(m_ask_price_levels[symbol][2]);
    m_latest_l2_tick->set_sell_price_1000x_4(m_ask_price_levels[symbol][3]);
    m_latest_l2_tick->set_sell_price_1000x_5(m_ask_price_levels[symbol][4]);
    m_latest_l2_tick->set_sell_quantity_1(m_ask_quantity_levels[symbol][0]);
    m_latest_l2_tick->set_sell_quantity_2(m_ask_quantity_levels[symbol][1]);
    m_latest_l2_tick->set_sell_quantity_3(m_ask_quantity_levels[symbol][2]);
    m_latest_l2_tick->set_sell_quantity_4(m_ask_quantity_levels[symbol][3]);
    m_latest_l2_tick->set_sell_quantity_5(m_ask_quantity_levels[symbol][4]);

    m_latest_l2_tick->set_buy_price_1000x_1(m_bid_price_levels[symbol][0]);
    m_latest_l2_tick->set_buy_price_1000x_2(m_bid_price_levels[symbol][1]);
    m_latest_l2_tick->set_buy_price_1000x_3(m_bid_price_levels[symbol][2]);
    m_latest_l2_tick->set_buy_price_1000x_4(m_bid_price_levels[symbol][3]);
    m_latest_l2_tick->set_buy_price_1000x_5(m_bid_price_levels[symbol][4]);
    m_latest_l2_tick->set_buy_quantity_1(m_bid_quantity_levels[symbol][0]);
    m_latest_l2_tick->set_buy_quantity_2(m_bid_quantity_levels[symbol][1]);
    m_latest_l2_tick->set_buy_quantity_3(m_bid_quantity_levels[symbol][2]);
    m_latest_l2_tick->set_buy_quantity_4(m_bid_quantity_levels[symbol][3]);
    m_latest_l2_tick->set_buy_quantity_5(m_bid_quantity_levels[symbol][4]);

    /// Order data.
    if (order->is_buy()) {
        m_latest_l2_tick->set_ask_unique_id(matched_order->unique_id());
        m_latest_l2_tick->set_bid_unique_id(order->unique_id());

        m_active_number[order->symbol()].buy++;
        m_active_traded_quantity[order->symbol()].buy += BookerCommonData::to_quantity(fill_qty);
        m_active_traded_amount_1000x[order->symbol()].buy += BookerCommonData::to_quantity(fill_qty) * BookerCommonData::to_price(fill_price);
    }
    else {
        m_latest_l2_tick->set_ask_unique_id(order->unique_id());
        m_latest_l2_tick->set_bid_unique_id(matched_order->unique_id());

        m_active_number[order->symbol()].sell++;
        m_active_traded_quantity[order->symbol()].sell += BookerCommonData::to_quantity(fill_qty);
        m_active_traded_amount_1000x[order->symbol()].sell += BookerCommonData::to_quantity(fill_qty) * BookerCommonData::to_price(fill_price);
    }

    m_latest_l2_tick->set_exchange_time(order->exchange_time());

    /// Active order data.
    m_latest_l2_tick->set_active_sell_number(m_active_number[symbol].sell);
    m_latest_l2_tick->set_active_sell_number_in_queue(m_ask_queue_size[symbol]);
    m_latest_l2_tick->set_active_sell_quantity(m_active_traded_quantity[symbol].sell);
    m_latest_l2_tick->set_active_sell_amount_1000x(m_active_traded_amount_1000x[symbol].sell);

    m_latest_l2_tick->set_active_buy_number(m_active_number[symbol].buy);
    m_latest_l2_tick->set_active_buy_number_in_queue(m_bid_queue_size[symbol]);
    m_latest_l2_tick->set_active_buy_quantity(m_active_traded_quantity[symbol].buy);
    m_latest_l2_tick->set_active_buy_amount_1000x(m_active_traded_amount_1000x[symbol].buy);

    m_latest_l2_tick->set_weighted_sell_price_5_1000x(calculate_weighted_price_1000x(symbol, m_ask_price_levels[symbol][4], types::SideType::sell));
    m_latest_l2_tick->set_weighted_sell_price_4_1000x(calculate_weighted_price_1000x(symbol, m_ask_price_levels[symbol][3], types::SideType::sell));
    m_latest_l2_tick->set_weighted_sell_price_3_1000x(calculate_weighted_price_1000x(symbol, m_ask_price_levels[symbol][2], types::SideType::sell));
    m_latest_l2_tick->set_weighted_sell_price_2_1000x(calculate_weighted_price_1000x(symbol, m_ask_price_levels[symbol][1], types::SideType::sell));
    m_latest_l2_tick->set_weighted_sell_price_1_1000x(calculate_weighted_price_1000x(symbol, m_ask_price_levels[symbol][0], types::SideType::sell));
    m_latest_l2_tick->set_weighted_buy_price_1_1000x(calculate_weighted_price_1000x(symbol, m_bid_price_levels[symbol][0], types::SideType::buy));
    m_latest_l2_tick->set_weighted_buy_price_2_1000x(calculate_weighted_price_1000x(symbol, m_bid_price_levels[symbol][1], types::SideType::buy));
    m_latest_l2_tick->set_weighted_buy_price_3_1000x(calculate_weighted_price_1000x(symbol, m_bid_price_levels[symbol][2], types::SideType::buy));
    m_latest_l2_tick->set_weighted_buy_price_4_1000x(calculate_weighted_price_1000x(symbol, m_bid_price_levels[symbol][3], types::SideType::buy));
    m_latest_l2_tick->set_weighted_buy_price_5_1000x(calculate_weighted_price_1000x(symbol, m_bid_price_levels[symbol][4], types::SideType::buy));
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

int64_t trade::booker::Booker::calculate_weighted_price_1000x(
    const std::string& symbol,
    const int64_t order_price_1000x,
    const types::SideType side
)
{
    assert(side == types::SideType::buy || side == types::SideType::sell);

    /// For arithmetic devision.
    const auto divides = [](const auto& x, const auto& y) {
        return std::divides<double>()(x, y);
    };

    double weighted_price = 0.;

    if (order_price_1000x == 0 || m_previous_price_1000x_1[symbol].sell == 0)
        return 0;

    switch (side) {
    case types::SideType::buy: weighted_price = 1. - std::tanh((divides(m_previous_price_1000x_1[symbol].buy, order_price_1000x) - 1) * 100); break;
    case types::SideType::sell: weighted_price = 1. - std::tanh((divides(order_price_1000x, m_previous_price_1000x_1[symbol].sell) - 1) * 100); break;
    default: break;
    }

    return static_cast<int64_t>(weighted_price * 1000.);
}
