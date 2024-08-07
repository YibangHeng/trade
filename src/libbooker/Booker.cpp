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
        generated_l2_tick->set_exchange_date(trade_tick->exchange_date());
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

    latest_l2_tick->set_exchange_date(order->exchange_date());
    latest_l2_tick->set_exchange_time(order->exchange_time());

    if (m_enable_advanced_calculating)
        add_range_snap(order, matched_order, fill_qty, fill_price);

    /// Booker::on_trade() is called after Booker::on_fill().
}

void trade::booker::Booker::on_cancel_reject(const OrderWrapperPtr& order, const char* reason)
{
    /// Do not log cancel rejections for failed symbols.
    if (!m_failed_symbols.contains(order->symbol()))
        logger->error("{}'s cancel for {} was rejected: {}", order->symbol(), order->unique_id(), reason);
}

void trade::booker::Booker::on_replace_reject(const OrderWrapperPtr& order, const char* reason)
{
    /// Do not log replace rejections for failed symbols.
    if (!m_failed_symbols.contains(order->symbol()))
        logger->error("{}'s replace for {} was rejected: {}", order->symbol(), order->unique_id(), reason);
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
    order_tick->set_exchange_date(trade_tick->exchange_date());
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
    order_tick->set_exchange_date(m_market_order[trade_tick->symbol()]->exchange_date());
    order_tick->set_exchange_time(m_market_order[trade_tick->symbol()]->exchange_time());

    /// For avoiding issus of deplicated order.
    m_orders.erase(m_market_order[trade_tick->symbol()]->unique_id());

    return order_tick;
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

void trade::booker::Booker::refresh_range(
    const std::string& symbol,
    const int64_t exchange_date,
    const int64_t exchange_time
)
{
    /// Ranged time starts from 93000000.
    if (!m_latest_ranged_time.contains(symbol)) [[unlikely]] {
        m_latest_ranged_time[symbol] = 93000000;
        return;
    }

    /// No need for refreshing until next ranged time.
    if (align_time(exchange_time) <= m_latest_ranged_time[symbol])
        return;

    m_latest_ranged_time[symbol] = align_time(exchange_time);

    /// Remove ranged ticks that not in wanted time range.
    const auto subrange = std::ranges::remove_if(m_ranged_ticks[symbol], [&](const auto& tick) {
        return tick->exchange_time() < minus_3_seconds(exchange_time);
    });
    m_ranged_ticks[symbol].erase(subrange.begin(), m_ranged_ticks[symbol].end());

    const auto generated_ranged_tick = std::make_shared<types::RangedTick>();

    /// Common data.
    generated_ranged_tick->set_symbol(symbol);
    generated_ranged_tick->set_exchange_date(exchange_date);
    generated_ranged_tick->set_exchange_time(align_time(exchange_time)); /// Use aligned time.

    /// Level prices.
    generate_level_price(symbol, generated_ranged_tick);

    /// Weighted prices.
    const auto& latest_l2_prices = std::make_shared<types::GeneratedL2Tick>();

    generate_level_price(symbol, latest_l2_prices);

    if (m_privious_l2_prices.contains(symbol)) {
        const auto& privious_l2_prices = m_privious_l2_prices[symbol];
        generate_weighted_price(latest_l2_prices, privious_l2_prices, generated_ranged_tick);
    }
    else {
        generate_weighted_price(latest_l2_prices, nullptr, generated_ranged_tick);
    }

    m_privious_l2_prices[symbol] = latest_l2_prices;

    if (m_ranged_ticks[symbol].empty()) {
        /// TODO: Set time and other filds here.
        generated_ranged_tick->set_start_time(minus_3_seconds(align_time(exchange_time)));
        generated_ranged_tick->set_end_time(align_time(exchange_time));

        m_reporter->ranged_tick_generated(generated_ranged_tick);
    }
    else {
        const auto first_ranged_tick = m_ranged_ticks[symbol].front();

        generated_ranged_tick->set_start_time(INT_MAX);
        generated_ranged_tick->set_end_time(INT_MIN);
        generated_ranged_tick->set_highest_price_1000x(INT_MIN);
        generated_ranged_tick->set_lowest_price_1000x(INT_MAX);
        generated_ranged_tick->set_ask_price_1_valid_duration_1000x(3010);
        generated_ranged_tick->set_bid_price_1_valid_duration_1000x(3010);

        /// Initial price 1.
        const int64_t init_ask_price_1 = m_ranged_ticks[symbol].front()->x_ask_price_1_1000x();
        const int64_t init_bid_price_1 = m_ranged_ticks[symbol].front()->x_bid_price_1_1000x();

        /// Calculate ranged data.
        for (const auto& ranged_tick : m_ranged_ticks[symbol]) {
            generated_ranged_tick->set_start_time(std::min(generated_ranged_tick->start_time(), ranged_tick->exchange_time()));
            generated_ranged_tick->set_end_time(std::max(generated_ranged_tick->end_time(), ranged_tick->exchange_time()));

            generated_ranged_tick->set_active_traded_sell_number(generated_ranged_tick->active_traded_sell_number() + ranged_tick->active_traded_sell_number());
            generated_ranged_tick->set_active_sell_number(generated_ranged_tick->active_sell_number() + ranged_tick->active_sell_number());
            generated_ranged_tick->set_active_sell_quantity(generated_ranged_tick->active_sell_quantity() + ranged_tick->active_sell_quantity());
            generated_ranged_tick->set_active_sell_amount_1000x(generated_ranged_tick->active_sell_amount_1000x() + ranged_tick->active_sell_amount_1000x());
            generated_ranged_tick->set_active_traded_buy_number(generated_ranged_tick->active_traded_buy_number() + ranged_tick->active_traded_buy_number());
            generated_ranged_tick->set_active_buy_number(generated_ranged_tick->active_buy_number() + ranged_tick->active_buy_number());
            generated_ranged_tick->set_active_buy_quantity(generated_ranged_tick->active_buy_quantity() + ranged_tick->active_buy_quantity());
            generated_ranged_tick->set_active_buy_amount_1000x(generated_ranged_tick->active_buy_amount_1000x() + ranged_tick->active_buy_amount_1000x());

            generated_ranged_tick->set_aggressive_sell_number(generated_ranged_tick->aggressive_sell_number() + ranged_tick->aggressive_sell_number());
            generated_ranged_tick->set_aggressive_buy_number(generated_ranged_tick->aggressive_buy_number() + ranged_tick->aggressive_buy_number());
            generated_ranged_tick->set_new_added_ask_1_quantity(generated_ranged_tick->new_added_ask_1_quantity() + ranged_tick->new_added_ask_1_quantity());
            generated_ranged_tick->set_new_added_bid_1_quantity(generated_ranged_tick->new_added_bid_1_quantity() + ranged_tick->new_added_bid_1_quantity());
            generated_ranged_tick->set_new_canceled_ask_1_quantity(generated_ranged_tick->new_canceled_ask_1_quantity() + ranged_tick->new_canceled_ask_1_quantity());
            generated_ranged_tick->set_new_canceled_bid_1_quantity(generated_ranged_tick->new_canceled_bid_1_quantity() + ranged_tick->new_canceled_bid_1_quantity());
            generated_ranged_tick->set_new_canceled_ask_all_quantity(generated_ranged_tick->new_canceled_ask_all_quantity() + ranged_tick->new_canceled_ask_all_quantity());
            generated_ranged_tick->set_new_canceled_bid_all_quantity(generated_ranged_tick->new_canceled_bid_all_quantity() + ranged_tick->new_canceled_bid_all_quantity());
            generated_ranged_tick->set_big_ask_amount_1000x(generated_ranged_tick->big_ask_amount_1000x() + ranged_tick->big_ask_amount_1000x());
            generated_ranged_tick->set_big_bid_amount_1000x(generated_ranged_tick->big_bid_amount_1000x() + ranged_tick->big_bid_amount_1000x());

            generated_ranged_tick->set_highest_price_1000x(std::max(generated_ranged_tick->highest_price_1000x(), ranged_tick->highest_price_1000x()));
            generated_ranged_tick->set_lowest_price_1000x(std::min(generated_ranged_tick->lowest_price_1000x(), ranged_tick->lowest_price_1000x()));

            if (ranged_tick->x_ask_price_1_1000x() > init_ask_price_1 && !generated_ranged_tick->ask_price_1_valid_duration_1000x() != 3010)
                generated_ranged_tick->set_ask_price_1_valid_duration_1000x(ranged_tick->exchange_time() - generated_ranged_tick->start_time());
            if (ranged_tick->x_bid_price_1_1000x() < init_bid_price_1 && !generated_ranged_tick->bid_price_1_valid_duration_1000x() != 3010)
                generated_ranged_tick->set_bid_price_1_valid_duration_1000x(ranged_tick->exchange_time() - generated_ranged_tick->start_time());
        }
    }

    /// Clear ranged ticks.
    m_ranged_ticks[symbol].clear();

    m_reporter->ranged_tick_generated(generated_ranged_tick);
}

void trade::booker::Booker::add_range_snap(const OrderTickPtr& order_tick)
{
    const int64_t time = order_tick->exchange_time();

    if (!((time >= 93000000 && time <= 113000000) || (time >= 130000000 && time <= 150000000)))
        return;

    const auto ranged_tick = std::make_shared<types::RangedTick>();

    ranged_tick->set_symbol(order_tick->symbol());
    ranged_tick->set_exchange_date(order_tick->exchange_date());
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

    refresh_range(order_tick->symbol(), order_tick->exchange_date(), time);
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

    const auto ranged_tick = std::make_shared<types::RangedTick>();

    ranged_tick->set_symbol(order->symbol());
    ranged_tick->set_exchange_date(order->exchange_date());
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

    refresh_range(order->symbol(), order->exchange_date(), time);
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

    /// Make sure weighted price is not empty.
    if (previous_l2_tick == nullptr) {
        for (int i = 0; i < 5; i++) {
            ranged_tick->add_weighted_ask_price(0);
            ranged_tick->add_weighted_bid_price(0);
        }

        return;
    }

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

int64_t trade::booker::Booker::minus_3_seconds(const int64_t time)
{
    /// Extract hours, minutes, and seconds from the input time.
    int64_t hours              = time / 10000000;
    int64_t minutes            = time / 100000 % 100;
    int64_t seconds            = time / 1000 % 100;

    const int64_t milliseconds = time % 1000;

    /// Subtract 3 seconds.
    seconds -= 3;

    /// Handle underflow of seconds.
    if (seconds < 0) {
        seconds += 60;
        minutes -= 1;
    }

    /// Handle underflow of minutes.
    if (minutes < 0) {
        minutes += 60;
        hours -= 1;
    }

    /// Handle underflow of hours.
    if (hours < 0) {
        hours += 24;
    }

    /// Reconstruct the time.
    return hours * 10000000 + minutes * 100000 + seconds * 1000 + milliseconds;
}
