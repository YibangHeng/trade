#pragma once

#include "RawStructure.h"
#include "libbooker/BookerCommonData.h"
#include "networks.pb.h"
#include "third/cut/UTApiStruct.h"

namespace trade::broker
{

template<typename T>
concept IsOrderTick = std::is_same_v<T, SSEHpfTick> || std::is_same_v<T, SZSEHpfOrderTick>;

template<typename T>
concept IsTradeTick = std::is_same_v<T, SZSEHpfTradeTick>;

template<typename T>
concept IsMdTrade = std::is_same_v<T, SSEHpfL2Snap> || std::is_same_v<T, SZSEHpfL2Snap>;

class CUTCommonData
{
public:
    CUTCommonData();
    ~CUTCommonData() = default;

public:
    [[nodiscard]] static TUTExchangeIDType to_exchange(types::ExchangeType exchange);
    [[nodiscard]] static types::ExchangeType to_exchange(TUTExchangeIDType exchange);
    [[nodiscard]] static types::OrderType to_order_type_from_sse(char tick_type);
    [[nodiscard]] static types::OrderType to_order_type_from_szse(char order_type);
    [[nodiscard]] static TUTDirectionType to_side(types::SideType side);
    [[nodiscard]] static types::SideType to_side(TUTDirectionType side);
    [[nodiscard]] static types::SideType to_md_side_from_sse(char side);
    [[nodiscard]] static types::SideType to_md_side_from_szse(char side);
    [[nodiscard]] static TUTOffsetFlagType to_position_side(types::PositionSideType position_side);
    [[nodiscard]] static types::PositionSideType to_position_side(TUTOffsetFlagType position_side);
    /// Concatenate front_id, session_id and order_ref to broker_id in format of
    /// {front_id}:{session_id}:{order_ref}.
    /// This tuples uniquely identify a CUT order.
    [[nodiscard]] static std::string to_broker_id(
        TUTFrontIDType front_id,
        TUTSessionIDType session_id,
        int order_ref
    );
    /// Extract front_id, session_id and order_ref from broker_id in format of
    /// {front_id}:{session_id}:{order_ref}.
    /// @return std::tuple<front_id, session_id, order_ref>. Empty string if
    /// broker_id is not in format.
    [[nodiscard]] static std::tuple<std::string, std::string, std::string> from_broker_id(const std::string& broker_id);
    /// Concatenate exchange and order_sys_id to exchange_id in format of
    /// {exchange}:{order_sys_id}.
    /// This tuples uniquely identify a CUT order.
    [[nodiscard]] static std::string to_exchange_id(
        TUTExchangeIDType exchange,
        const std::string& order_sys_id
    );
    /// Extract exchange and order_sys_id from exchange_id in format of
    /// {exchange}:{order_sys_id}.
    /// @return std::tuple<exchange, order_sys_id>. Empty string if exchange_id
    /// is not in format.
    [[nodiscard]] static std::tuple<std::string, std::string> from_exchange_id(const std::string& exchange_id);
    [[nodiscard]] static int64_t get_symbol_from_message(const std::vector<u_char>& message);
    template<IsOrderTick MessageType>
    [[nodiscard]] static booker::OrderTickPtr to_order_tick(const std::vector<u_char>& message);
    template<IsTradeTick Mess7ageType>
    [[nodiscard]] static booker::TradeTickPtr to_trade_tick(const std::vector<u_char>& message);
    template<IsMdTrade MessageType>
    [[nodiscard]] static booker::ExchangeL2SnapPtr to_l2_tick(const std::vector<u_char>& message);

    [[nodiscard]] static booker::TradeTickPtr x_ost_forward_to_trade_from_order(booker::OrderTickPtr& order_tick);
    [[nodiscard]] static booker::OrderTickPtr x_ost_forward_to_order_from_trade(booker::TradeTickPtr& trade_tick);

    [[nodiscard]] static int64_t to_price_1000x_from_sse(uint32_t order_price);
    [[nodiscard]] static int64_t to_price_1000x_from_szse(uint32_t exe_px);
    [[nodiscard]] static int64_t to_quantity_from_sse(uint32_t qty);
    [[nodiscard]] static int64_t to_quantity_from_szse(uint32_t exe_qty);
    [[nodiscard]] static int64_t to_date_from_sse(const uint16_t year, const uint8_t month, const uint8_t day);
    [[nodiscard]] static int64_t to_time_from_sse(uint32_t tick_time);
    [[nodiscard]] static int64_t to_date_from_szse(uint64_t quote_update_time);
    [[nodiscard]] static int64_t to_time_from_szse(uint64_t quote_update_time);

    /// Append symbol info to exchange_raw id as prefix.
    template<std::integral IdType>
    [[nodiscard]] static int64_t to_unique_id(IdType exchange_raw_id, std::string_view symbol);

public:
    TUTSystemNameType m_system_name;
    TUTUserIDType m_user_id;
    TUTInvestorIDType m_investor_id;
    TUTFrontIDType m_front_id;
    TUTSessionIDType m_session_id;
};

template<>
inline booker::OrderTickPtr CUTCommonData::to_order_tick<SSEHpfTick>(const std::vector<u_char>& message)
{
    auto order_tick = std::make_shared<types::OrderTick>();

    assert(message.size() == sizeof(SSEHpfTick));
    const auto raw_order = reinterpret_cast<const SSEHpfTick*>(message.data());

    order_tick->set_unique_id(to_unique_id(raw_order->m_buy_order_no + raw_order->m_sell_order_no, raw_order->m_symbol_id));
    order_tick->set_order_type(to_order_type_from_sse(raw_order->m_tick_type));
    order_tick->set_symbol(raw_order->m_symbol_id);
    order_tick->set_side(to_md_side_from_sse(raw_order->m_side_flag));
    order_tick->set_price_1000x(to_price_1000x_from_sse(raw_order->m_order_price));
    order_tick->set_quantity(to_quantity_from_sse(raw_order->m_qty));
    order_tick->set_exchange_date(to_date_from_sse(raw_order->m_head.m_data_year, raw_order->m_head.m_data_month, raw_order->m_head.m_data_day));
    order_tick->set_exchange_time(to_time_from_sse(raw_order->m_tick_time));
    order_tick->set_x_ost_sse_ask_unique_id(to_unique_id(raw_order->m_sell_order_no, raw_order->m_symbol_id));
    order_tick->set_x_ost_sse_bid_unique_id(to_unique_id(raw_order->m_buy_order_no, raw_order->m_symbol_id));

    if (order_tick->order_type() == types::OrderType::invalid_order_type)
        return nullptr;

    return order_tick;
}

template<>
inline booker::OrderTickPtr CUTCommonData::to_order_tick<SZSEHpfOrderTick>(const std::vector<u_char>& message)
{
    auto order_tick = std::make_shared<types::OrderTick>();

    assert(message.size() == sizeof(SZSEHpfOrderTick));
    const auto raw_order = reinterpret_cast<const SZSEHpfOrderTick*>(message.data());

    order_tick->set_unique_id(to_unique_id(raw_order->m_header.m_sequence_num, raw_order->m_header.m_symbol));
    order_tick->set_order_type(to_order_type_from_szse(raw_order->m_order_type));
    order_tick->set_symbol(raw_order->m_header.m_symbol);
    order_tick->set_side(to_md_side_from_szse(raw_order->m_side));
    order_tick->set_price_1000x(to_price_1000x_from_szse(raw_order->m_px));
    order_tick->set_quantity(to_quantity_from_szse(raw_order->m_qty));
    order_tick->set_exchange_date(to_date_from_szse(raw_order->m_header.m_quote_update_time));
    order_tick->set_exchange_time(to_time_from_szse(raw_order->m_header.m_quote_update_time));

    /// Just ignore fill tick.
    if (order_tick->order_type() == types::OrderType::invalid_order_type
        || order_tick->order_type() == types::OrderType::fill)
        return nullptr;

    return order_tick;
}

template<>
inline booker::TradeTickPtr CUTCommonData::to_trade_tick<SZSEHpfTradeTick>(const std::vector<u_char>& message)
{
    auto trade_tick = std::make_shared<types::TradeTick>();

    assert(message.size() == sizeof(SZSEHpfTradeTick));
    const auto raw_trade = reinterpret_cast<const SZSEHpfTradeTick*>(message.data());

    trade_tick->set_ask_unique_id(to_unique_id(raw_trade->m_ask_app_seq_num, raw_trade->m_header.m_symbol));
    trade_tick->set_bid_unique_id(to_unique_id(raw_trade->m_bid_app_seq_num, raw_trade->m_header.m_symbol));
    trade_tick->set_symbol(raw_trade->m_header.m_symbol);
    trade_tick->set_exec_price_1000x(to_price_1000x_from_szse(raw_trade->m_exe_px));
    trade_tick->set_exec_quantity(to_quantity_from_szse(raw_trade->m_exe_qty));
    trade_tick->set_exchange_date(to_date_from_szse(raw_trade->m_header.m_quote_update_time));
    trade_tick->set_exchange_time(to_time_from_szse(raw_trade->m_header.m_quote_update_time));
    trade_tick->set_x_ost_szse_exe_type(to_order_type_from_szse(raw_trade->m_exe_type));

    if (trade_tick.unique() == 0
        || trade_tick->x_ost_szse_exe_type() == types::OrderType::invalid_order_type) {
        return nullptr;
    }

    return trade_tick;
}

template<>
inline booker::ExchangeL2SnapPtr CUTCommonData::to_l2_tick<SSEHpfL2Snap>(const std::vector<u_char>& message)
{
    auto exchange_l2_snap = std::make_shared<types::ExchangeL2Snap>();

    assert(message.size() == sizeof(SSEHpfL2Snap));
    const auto raw_l2_tick = reinterpret_cast<const SSEHpfL2Snap*>(message.data());

    exchange_l2_snap->set_symbol(raw_l2_tick->m_symbol_id);
    exchange_l2_snap->set_price(raw_l2_tick->m_last_price / 1000.);

    exchange_l2_snap->set_pre_settlement(raw_l2_tick->m_prev_close);
    exchange_l2_snap->set_pre_close_price(raw_l2_tick->m_prev_close);
    exchange_l2_snap->set_open_price(raw_l2_tick->m_open_price);
    exchange_l2_snap->set_highest_price(raw_l2_tick->m_day_high);
    exchange_l2_snap->set_lowest_price(raw_l2_tick->m_day_low);
    exchange_l2_snap->set_close_price(raw_l2_tick->m_last_price);
    exchange_l2_snap->set_settlement_price(raw_l2_tick->m_close_price);
    exchange_l2_snap->set_upper_limit_price(raw_l2_tick->m_last_price * 1.1 / 1000.);
    exchange_l2_snap->set_lower_limit_price(raw_l2_tick->m_last_price * 0.9 / 1000.);

    exchange_l2_snap->set_sell_price_1(raw_l2_tick->m_ask_px[0].m_px / 1000.);
    exchange_l2_snap->set_sell_quantity_1(to_quantity_from_sse(raw_l2_tick->m_ask_px[0].m_qty));
    exchange_l2_snap->set_sell_price_2(raw_l2_tick->m_ask_px[1].m_px / 1000.);
    exchange_l2_snap->set_sell_quantity_2(to_quantity_from_sse(raw_l2_tick->m_ask_px[1].m_qty));
    exchange_l2_snap->set_sell_price_3(raw_l2_tick->m_ask_px[2].m_px / 1000.);
    exchange_l2_snap->set_sell_quantity_3(to_quantity_from_sse(raw_l2_tick->m_ask_px[2].m_qty));
    exchange_l2_snap->set_sell_price_4(raw_l2_tick->m_ask_px[3].m_px / 1000.);
    exchange_l2_snap->set_sell_quantity_4(to_quantity_from_sse(raw_l2_tick->m_ask_px[3].m_qty));
    exchange_l2_snap->set_sell_price_5(raw_l2_tick->m_ask_px[4].m_px / 1000.);
    exchange_l2_snap->set_sell_quantity_5(to_quantity_from_sse(raw_l2_tick->m_ask_px[4].m_qty));

    exchange_l2_snap->set_buy_price_1(raw_l2_tick->m_bid_px[0].m_px / 1000.);
    exchange_l2_snap->set_buy_quantity_1(to_quantity_from_sse(raw_l2_tick->m_bid_px[0].m_qty));
    exchange_l2_snap->set_buy_price_2(raw_l2_tick->m_bid_px[1].m_px / 1000.);
    exchange_l2_snap->set_buy_quantity_2(to_quantity_from_sse(raw_l2_tick->m_bid_px[1].m_qty));
    exchange_l2_snap->set_buy_price_3(raw_l2_tick->m_bid_px[2].m_px / 1000.);
    exchange_l2_snap->set_buy_quantity_3(to_quantity_from_sse(raw_l2_tick->m_bid_px[2].m_qty));
    exchange_l2_snap->set_buy_price_4(raw_l2_tick->m_bid_px[3].m_px / 1000.);
    exchange_l2_snap->set_buy_quantity_4(to_quantity_from_sse(raw_l2_tick->m_bid_px[3].m_qty));
    exchange_l2_snap->set_buy_price_5(raw_l2_tick->m_bid_px[4].m_px / 1000.);
    exchange_l2_snap->set_buy_quantity_5(to_quantity_from_sse(raw_l2_tick->m_bid_px[4].m_qty));

    return exchange_l2_snap;
}

template<>
inline booker::ExchangeL2SnapPtr CUTCommonData::to_l2_tick<SZSEHpfL2Snap>(const std::vector<u_char>& message)
{
    auto exchange_l2_snap = std::make_shared<types::ExchangeL2Snap>();

    assert(message.size() == sizeof(SZSEHpfL2Snap));
    const auto raw_l2_tick = reinterpret_cast<const SZSEHpfL2Snap*>(message.data());

    exchange_l2_snap->set_symbol(raw_l2_tick->m_header.m_symbol);
    exchange_l2_snap->set_price(raw_l2_tick->m_last_price / 10000.);

    exchange_l2_snap->set_pre_settlement(raw_l2_tick->m_previous_close_price / 10000.);
    exchange_l2_snap->set_pre_close_price(raw_l2_tick->m_previous_close_price / 10000.);
    exchange_l2_snap->set_open_price(raw_l2_tick->m_open_price / 10000.);
    exchange_l2_snap->set_highest_price(raw_l2_tick->m_day_high / 10000.);
    exchange_l2_snap->set_lowest_price(raw_l2_tick->m_day_low / 10000.);
    exchange_l2_snap->set_close_price(raw_l2_tick->m_today_close_price / 10000.);
    exchange_l2_snap->set_settlement_price(raw_l2_tick->m_today_close_price / 10000.);
    exchange_l2_snap->set_upper_limit_price(raw_l2_tick->m_upper_limit_price / 10000.);
    exchange_l2_snap->set_lower_limit_price(raw_l2_tick->m_lower_limit_price / 10000.);

    exchange_l2_snap->set_sell_price_1(raw_l2_tick->m_ask_unit[0].m_price / 10000.);
    exchange_l2_snap->set_sell_quantity_1(to_quantity_from_szse(raw_l2_tick->m_ask_unit[0].m_qty));
    exchange_l2_snap->set_sell_price_2(raw_l2_tick->m_ask_unit[1].m_price / 10000.);
    exchange_l2_snap->set_sell_quantity_2(to_quantity_from_szse(raw_l2_tick->m_ask_unit[1].m_qty));
    exchange_l2_snap->set_sell_price_3(raw_l2_tick->m_ask_unit[2].m_price / 10000.);
    exchange_l2_snap->set_sell_quantity_3(to_quantity_from_szse(raw_l2_tick->m_ask_unit[2].m_qty));
    exchange_l2_snap->set_sell_price_4(raw_l2_tick->m_ask_unit[3].m_price / 10000.);
    exchange_l2_snap->set_sell_quantity_4(to_quantity_from_szse(raw_l2_tick->m_ask_unit[3].m_qty));
    exchange_l2_snap->set_sell_price_5(raw_l2_tick->m_ask_unit[4].m_price / 10000.);
    exchange_l2_snap->set_sell_quantity_5(to_quantity_from_szse(raw_l2_tick->m_ask_unit[4].m_qty));

    exchange_l2_snap->set_buy_price_1(raw_l2_tick->m_bid_unit[0].m_price / 10000.);
    exchange_l2_snap->set_buy_quantity_1(to_quantity_from_szse(raw_l2_tick->m_bid_unit[0].m_qty));
    exchange_l2_snap->set_buy_price_2(raw_l2_tick->m_bid_unit[1].m_price / 10000.);
    exchange_l2_snap->set_buy_quantity_2(to_quantity_from_szse(raw_l2_tick->m_bid_unit[1].m_qty));
    exchange_l2_snap->set_buy_price_3(raw_l2_tick->m_bid_unit[2].m_price / 10000.);
    exchange_l2_snap->set_buy_quantity_3(to_quantity_from_szse(raw_l2_tick->m_bid_unit[2].m_qty));
    exchange_l2_snap->set_buy_price_4(raw_l2_tick->m_bid_unit[3].m_price / 10000.);
    exchange_l2_snap->set_buy_quantity_4(to_quantity_from_szse(raw_l2_tick->m_bid_unit[3].m_qty));
    exchange_l2_snap->set_buy_price_5(raw_l2_tick->m_bid_unit[4].m_price / 10000.);
    exchange_l2_snap->set_buy_quantity_5(to_quantity_from_szse(raw_l2_tick->m_bid_unit[4].m_qty));

    return exchange_l2_snap;
}

template<std::integral T>
int64_t CUTCommonData::to_unique_id(T exchange_raw_id, const std::string_view symbol)
{
    int64_t unique_id = static_cast<int64_t>(exchange_raw_id) * 1000000;

    try {
        unique_id += std::stoll(symbol.data());
    }
    catch (...) {
        /// If the symbol is not a number, we just ignore it.
    }

    return unique_id;
}

} // namespace trade::broker
