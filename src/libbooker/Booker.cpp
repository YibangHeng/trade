#include <ranges>

#include "libbooker/Booker.h"
#include "libbooker/BookerCommonData.h"
#include "utilities/TimeHelper.hpp"
#include "utilities/ToJSON.hpp"

trade::booker::Booker::Booker(
    const std::vector<std::string>& symbols,
    const std::shared_ptr<reporter::IReporter>& reporter,
    const bool enable_validation,
    const bool enable_advanced_calculating
) : AppBase("Booker"),
    m_in_continuous_stage(),
    m_md_validator(enable_validation ? MdValidator() : std::optional<MdValidator> {}),
    m_enable_advanced_calculating(enable_advanced_calculating),
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

    if (m_enable_advanced_calculating)
        add_range_snap(order_tick);
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

        /// Just make sure there are 5 levels.
        for (int i = 0; i < 5; i++) {
            generated_l2_tick->add_ask_levels();
            generated_l2_tick->add_bid_levels();
        }

        assert(generated_l2_tick->ask_levels_size() == 5 && generated_l2_tick->bid_levels_size() == 5);
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

    const auto latest_l2_tick = m_generated_l2_ticks[book->symbol()];

    latest_l2_tick->set_symbol(book->symbol());
    latest_l2_tick->set_price_1000x(BookerCommonData::to_price(price));
    latest_l2_tick->set_quantity(BookerCommonData::to_quantity(qty));

    latest_l2_tick->set_result(!m_failed_symbols.contains(book->symbol()));

    generate_level_price(book->symbol(), latest_l2_tick);

    m_md_validator.has_value() ? m_md_validator.value().l2_tick_generated(latest_l2_tick) : void(); /// Feed to validator first.

    assert(latest_l2_tick->ask_levels_size() == 5 && latest_l2_tick->bid_levels_size() == 5);
    m_reporter->l2_tick_generated(latest_l2_tick);
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
    const auto latest_l2_tick = std::make_shared<types::GeneratedL2Tick>();

    /// Store to m_generated_l2_ticks first.
    m_generated_l2_ticks[order->symbol()] = latest_l2_tick;

    if (order->is_buy()) {
        latest_l2_tick->set_ask_unique_id(matched_order->unique_id());
        latest_l2_tick->set_bid_unique_id(order->unique_id());
    }
    else {
        latest_l2_tick->set_ask_unique_id(order->unique_id());
        latest_l2_tick->set_bid_unique_id(matched_order->unique_id());
    }

    latest_l2_tick->set_exchange_time(order->exchange_time());

    if (m_enable_advanced_calculating)
        add_range_snap(order, matched_order, fill_qty, fill_price);

    /// Booker::on_trade() is called after Booker::on_fill().
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

void trade::booker::Booker::generate_level_price(
    const std::string& symbol,
    const GeneratedL2TickPtr& latest_l2_tick
)
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

    latest_l2_tick->clear_ask_levels();
    latest_l2_tick->clear_bid_levels();

    /// Assign price/quantity levels to latest l2 tick.
    for (int i = 0; i < 5; i++) {
        const auto ask_level = latest_l2_tick->add_ask_levels();
        ask_level->set_price_1000x(ask_price_levels[i]);
        ask_level->set_quantity(ask_quantity_levels[i]);

        const auto bid_level = latest_l2_tick->add_bid_levels();
        bid_level->set_price_1000x(bid_price_levels[i]);
        bid_level->set_quantity(bid_quantity_levels[i]);
    }
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

void trade::booker::Booker::refresh_range(const std::string& symbol, const int64_t time)
{
    if (m_ranged_ticks[symbol].empty())
        return;

    const auto first_ranged_tick = m_ranged_ticks[symbol].front();

    if (time - first_ranged_tick->exchange_time() < 3000)
        return;

    const auto latest_ranged_tick = std::make_shared<types::RangedTick>();

    latest_ranged_tick->set_symbol(first_ranged_tick->symbol());
    latest_ranged_tick->set_exchange_time(align_time(time)); /// Use aligned time.
    latest_ranged_tick->set_start_time(INT_MAX);
    latest_ranged_tick->set_end_time(INT_MIN);
    latest_ranged_tick->set_highest_price_1000x(INT_MIN);
    latest_ranged_tick->set_lowest_price_1000x(INT_MAX);
    latest_ranged_tick->set_ask_price_1_valid_duration_1000x(3010);
    latest_ranged_tick->set_bid_price_1_valid_duration_1000x(3010);

    /// Weighted prices.
    const auto& latest_l2_prices = std::make_shared<types::GeneratedL2Tick>();
    generate_level_price(symbol, latest_l2_prices);

    if (m_privious_l2_prices.contains(symbol)) {
        const auto& privious_l2_prices = m_privious_l2_prices[symbol];
        generate_weighted_price(latest_l2_prices, privious_l2_prices, latest_ranged_tick);
    }

    m_privious_l2_prices[symbol] = latest_l2_prices;

    /// Initial price 1.
    int64_t init_ask_price_1 = m_ranged_ticks[symbol].front()->x_ask_price_1_1000x();
    int64_t init_bid_price_1 = m_ranged_ticks[symbol].front()->x_bid_price_1_1000x();

    /// Calculate ranged data.
    for (const auto& ranged_tick : m_ranged_ticks[symbol]) {
        if (ranged_tick->exchange_time() < time - 3000)
            continue;

        latest_ranged_tick->set_start_time(std::min(latest_ranged_tick->start_time(), ranged_tick->exchange_time()));
        latest_ranged_tick->set_end_time(std::max(latest_ranged_tick->end_time(), ranged_tick->exchange_time()));

        latest_ranged_tick->set_active_traded_sell_number(latest_ranged_tick->active_traded_sell_number() + ranged_tick->active_traded_sell_number());
        latest_ranged_tick->set_active_sell_number(latest_ranged_tick->active_sell_number() + ranged_tick->active_sell_number());
        latest_ranged_tick->set_active_sell_quantity(latest_ranged_tick->active_sell_quantity() + ranged_tick->active_sell_quantity());
        latest_ranged_tick->set_active_sell_amount_1000x(latest_ranged_tick->active_sell_amount_1000x() + ranged_tick->active_sell_amount_1000x());
        latest_ranged_tick->set_active_traded_buy_number(latest_ranged_tick->active_traded_buy_number() + ranged_tick->active_traded_buy_number());
        latest_ranged_tick->set_active_buy_number(latest_ranged_tick->active_buy_number() + ranged_tick->active_buy_number());
        latest_ranged_tick->set_active_buy_quantity(latest_ranged_tick->active_buy_quantity() + ranged_tick->active_buy_quantity());
        latest_ranged_tick->set_active_buy_amount_1000x(latest_ranged_tick->active_buy_amount_1000x() + ranged_tick->active_buy_amount_1000x());

        latest_ranged_tick->set_aggressive_sell_number(latest_ranged_tick->aggressive_sell_number() + ranged_tick->aggressive_sell_number());
        latest_ranged_tick->set_aggressive_buy_number(latest_ranged_tick->aggressive_buy_number() + ranged_tick->aggressive_buy_number());
        latest_ranged_tick->set_new_added_ask_1_quantity(latest_ranged_tick->new_added_ask_1_quantity() + ranged_tick->new_added_ask_1_quantity());
        latest_ranged_tick->set_new_added_bid_1_quantity(latest_ranged_tick->new_added_bid_1_quantity() + ranged_tick->new_added_bid_1_quantity());
        latest_ranged_tick->set_new_canceled_ask_1_quantity(latest_ranged_tick->new_canceled_ask_1_quantity() + ranged_tick->new_canceled_ask_1_quantity());
        latest_ranged_tick->set_new_canceled_bid_1_quantity(latest_ranged_tick->new_canceled_bid_1_quantity() + ranged_tick->new_canceled_bid_1_quantity());
        latest_ranged_tick->set_new_canceled_ask_all_quantity(latest_ranged_tick->new_canceled_ask_all_quantity() + ranged_tick->new_canceled_ask_all_quantity());
        latest_ranged_tick->set_new_canceled_bid_all_quantity(latest_ranged_tick->new_canceled_bid_all_quantity() + ranged_tick->new_canceled_bid_all_quantity());
        latest_ranged_tick->set_big_ask_amount_1000x(latest_ranged_tick->big_ask_amount_1000x() + ranged_tick->big_ask_amount_1000x());
        latest_ranged_tick->set_big_bid_amount_1000x(latest_ranged_tick->big_bid_amount_1000x() + ranged_tick->big_bid_amount_1000x());

        latest_ranged_tick->set_highest_price_1000x(std::max(latest_ranged_tick->highest_price_1000x(), ranged_tick->highest_price_1000x()));
        latest_ranged_tick->set_lowest_price_1000x(std::min(latest_ranged_tick->lowest_price_1000x(), ranged_tick->lowest_price_1000x()));

        if (ranged_tick->x_ask_price_1_1000x() > init_ask_price_1 && !latest_ranged_tick->ask_price_1_valid_duration_1000x() != 3010)
            latest_ranged_tick->set_ask_price_1_valid_duration_1000x(ranged_tick->exchange_time() - latest_ranged_tick->start_time());
        if (ranged_tick->x_bid_price_1_1000x() < init_bid_price_1 && !latest_ranged_tick->bid_price_1_valid_duration_1000x() != 3010)
            latest_ranged_tick->set_bid_price_1_valid_duration_1000x(ranged_tick->exchange_time() - latest_ranged_tick->start_time());
    }

    /// Clear ranged ticks.
    m_ranged_ticks[symbol].clear();

    m_reporter->ranged_tick_generated(latest_ranged_tick);
}

void trade::booker::Booker::add_range_snap(const OrderTickPtr& order_tick)
{
    const int64_t time = order_tick->exchange_time();

    if (!((time >= 93000000 && time <= 113000000) || (time >= 130000000 && time <= 150000000)))
        return;

    /// Refresh range first.
    refresh_range(order_tick->symbol(), time);

    const auto ranged_tick = std::make_shared<types::RangedTick>();

    ranged_tick->set_symbol(order_tick->symbol());
    ranged_tick->set_exchange_time(time);

    switch (order_tick->order_type()) {
    case types::OrderType::limit: {
        switch (order_tick->side()) {
        case types::SideType::buy: {
            ranged_tick->set_active_buy_number(1); /// 主买报单笔数
            if (order_tick->price_1000x() > m_books[order_tick->symbol()]->bids().begin()->first.price())
                ranged_tick->set_aggressive_buy_number(1); /// 新增激进主买报单笔数
            if (order_tick->price_1000x() == m_books[order_tick->symbol()]->bids().begin()->first.price())
                ranged_tick->set_new_added_bid_1_quantity(order_tick->quantity()); /// 新增买一量合计
            break;
        }
        case types::SideType::sell: {
            ranged_tick->set_active_sell_number(1); /// 主卖报单笔数
            if (order_tick->price_1000x() < m_books[order_tick->symbol()]->asks().begin()->first.price())
                ranged_tick->set_aggressive_sell_number(1); /// 新增激进主卖报单笔数
            if (order_tick->price_1000x() == m_books[order_tick->symbol()]->asks().begin()->first.price())
                ranged_tick->set_new_added_ask_1_quantity(order_tick->quantity()); /// 新增卖一量合计
            break;
        }
        default: break;
        }
    }
    case types::OrderType::cancel: {
        switch (order_tick->side()) {
        case types::SideType::buy: {
            ranged_tick->set_new_canceled_bid_1_quantity(order_tick->quantity()); /// 新撤买一量合计
            break;
        }
        case types::SideType::sell: {
            ranged_tick->set_new_canceled_ask_1_quantity(order_tick->quantity()); /// 新撤卖一量合计
            break;
        }
        default: break;
        }
    }
    default: break;
    }

    ranged_tick->set_highest_price_1000x(INT_MIN);                                                                                  /// 行情最高价
    ranged_tick->set_lowest_price_1000x(INT_MAX);                                                                                   /// 行情最低价
    ranged_tick->set_x_ask_price_1_1000x(BookerCommonData::to_price(m_books[order_tick->symbol()]->bids().begin()->first.price())); /// 当前卖一价
    ranged_tick->set_x_bid_price_1_1000x(BookerCommonData::to_price(m_books[order_tick->symbol()]->asks().begin()->first.price())); /// 当前买一价

    m_ranged_ticks[order_tick->symbol()].push_back(ranged_tick);
}

void trade::booker::Booker::add_range_snap(
    const OrderWrapperPtr& order,
    const OrderWrapperPtr& matched_order,
    const liquibook::book::Quantity fill_qty,
    const liquibook::book::Price fill_price
)
{
    const int64_t time = order->exchange_time();

    if (!((time > 93000000 && time < 113000000) || (time > 130000000 && time < 150000000)))
        return;

    /// Refresh range first.
    refresh_range(order->symbol(), time);

    const auto ranged_tick = std::make_shared<types::RangedTick>();

    ranged_tick->set_symbol(order->symbol());
    ranged_tick->set_exchange_time(time);

    if (order->is_buy()) {
        ranged_tick->set_active_traded_buy_number(1);                                  /// 主买成交笔数
        ranged_tick->set_active_buy_quantity(BookerCommonData::to_quantity(fill_qty)); /// 主买成交数量
        ranged_tick->set_active_buy_amount_1000x(
            BookerCommonData::to_quantity(fill_qty)
            * BookerCommonData::to_price(fill_price)
        ); /// 主买成交金额
        if (ranged_tick->active_buy_amount_1000x() >= 50000000)
            ranged_tick->set_big_ask_amount_1000x(ranged_tick->active_buy_amount_1000x()); /// 大单买单成交金额
    }
    else {
        ranged_tick->set_active_traded_sell_number(1);                                  /// 主卖成交笔数
        ranged_tick->set_active_sell_quantity(BookerCommonData::to_quantity(fill_qty)); /// 主卖成交数量
        ranged_tick->set_active_sell_amount_1000x(
            BookerCommonData::to_quantity(fill_qty)
            * BookerCommonData::to_price(fill_price)
        ); /// 主卖成交金额
        if (ranged_tick->active_sell_amount_1000x() >= 50000000)
            ranged_tick->set_big_bid_amount_1000x(ranged_tick->active_sell_amount_1000x()); /// 大单卖单成交金额
    }

    ranged_tick->set_highest_price_1000x(BookerCommonData::to_price(fill_price));                                              /// 行情最高价
    ranged_tick->set_lowest_price_1000x(BookerCommonData::to_price(fill_price));                                               /// 行情最低价
    ranged_tick->set_x_ask_price_1_1000x(BookerCommonData::to_price(m_books[order->symbol()]->bids().begin()->first.price())); /// 当前卖一价
    ranged_tick->set_x_bid_price_1_1000x(BookerCommonData::to_price(m_books[order->symbol()]->asks().begin()->first.price())); /// 当前买一价

    m_ranged_ticks[order->symbol()].push_back(ranged_tick);
}

void trade::booker::Booker::generate_weighted_price(
    const GeneratedL2TickPtr& latest_l2_tick,
    const GeneratedL2TickPtr& previous_l2_tick,
    const RangedTickPtr& ranged_tick
)
{
    /// For arithmetic devision.
    const auto divides = [](const auto& x, const auto& y) {
        return std::divides<double>()(x, y);
    };

    int64_t previous_ask_price_1000x_1, previous_bid_price_1000x_1;

    if (!previous_l2_tick->ask_levels().empty())
        previous_ask_price_1000x_1 = previous_l2_tick->ask_levels().at(0).price_1000x();
    if (!previous_l2_tick->bid_levels().empty())
        previous_bid_price_1000x_1 = previous_l2_tick->bid_levels().at(0).price_1000x();

    for (int i = 0; i < 5; i++) {
        if (latest_l2_tick->ask_levels().size() >= i)
            ranged_tick->add_weighted_ask_price(1. - std::tanh((divides(latest_l2_tick->ask_levels().at(i).price_1000x(), previous_ask_price_1000x_1) - 1) * 100));
        if (latest_l2_tick->bid_levels().size() >= i)
            ranged_tick->add_weighted_bid_price(1. - std::tanh((divides(previous_bid_price_1000x_1, latest_l2_tick->bid_levels().at(i).price_1000x()) - 1) * 100));
    }
}

int64_t trade::booker::Booker::align_time(int64_t time)
{
    static std::array<int64_t, 20> tp = {0, 3, 6, 9, 12, 15, 18, 21, 24, 27, 30, 33, 36, 39, 42, 45, 48, 51, 54, 57};

    /// seconds for seconds in time.
    const auto seconds = time / 1000 % 100;
    time               = time / 1000 - seconds;

    for (const long& rit : std::ranges::reverse_view(tp)) {
        if (rit <= seconds) {
            time += rit;
            break;
        }
    }

    return time * 1000;
}
