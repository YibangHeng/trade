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
    template<IsOrderTick MessageType>
    [[nodiscard]] static booker::OrderTickPtr to_order_tick(const std::vector<u_char>& message);
    template<IsTradeTick Mess7ageType>
    [[nodiscard]] static booker::TradeTickPtr to_trade_tick(const std::vector<u_char>& message);
    template<IsMdTrade MessageType>
    [[nodiscard]] static booker::MdTradePtr to_md_trade(const std::vector<u_char>& message);
    [[nodiscard]] static double to_sse_price(uint32_t order_price);
    [[nodiscard]] static double to_szse_price(uint32_t exe_px);
    [[nodiscard]] static int64_t to_sse_quantity(uint32_t qty);
    [[nodiscard]] static int64_t to_szse_quantity(uint32_t exe_qty);
    [[nodiscard]] static int64_t to_sse_time(uint32_t tick_time);
    [[nodiscard]] static int64_t to_szse_time(uint64_t quote_update_time);

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

    order_tick->set_unique_id(static_cast<int64_t>(raw_order->m_buy_order_no + raw_order->m_sell_order_no));
    order_tick->set_order_type(to_order_type_from_sse(raw_order->m_tick_type));
    order_tick->set_symbol(raw_order->m_symbol_id);
    order_tick->set_side(to_md_side_from_sse(raw_order->m_side_flag));
    order_tick->set_price(to_sse_price(raw_order->m_order_price));
    order_tick->set_quantity(to_sse_quantity(raw_order->m_qty));
    order_tick->set_exchange_time(to_sse_time(raw_order->m_tick_time));

    return order_tick;
}

template<>
inline booker::OrderTickPtr CUTCommonData::to_order_tick<SZSEHpfOrderTick>(const std::vector<u_char>& message)
{
    auto order_tick = std::make_shared<types::OrderTick>();

    assert(message.size() == sizeof(SZSEHpfOrderTick));
    const auto raw_order = reinterpret_cast<const SZSEHpfOrderTick*>(message.data());

    order_tick->set_unique_id(raw_order->m_header.m_sequence_num);
    order_tick->set_order_type(to_order_type_from_szse(raw_order->m_order_type));
    order_tick->set_symbol(raw_order->m_header.m_symbol);
    order_tick->set_side(to_md_side_from_szse(raw_order->m_side));
    order_tick->set_price(to_szse_price(raw_order->m_px));
    order_tick->set_quantity(to_szse_quantity(raw_order->m_qty));
    order_tick->set_exchange_time(to_szse_time(raw_order->m_header.m_quote_update_time));

    return order_tick;
}

template<>
inline booker::TradeTickPtr CUTCommonData::to_trade_tick<SZSEHpfTradeTick>(const std::vector<u_char>& message)
{
    auto trade_tick = std::make_shared<types::TradeTick>();

    assert(message.size() == sizeof(SZSEHpfTradeTick));
    const auto raw_trade = reinterpret_cast<const SZSEHpfTradeTick*>(message.data());

    trade_tick->set_ask_unique_id(raw_trade->m_ask_app_seq_num);
    trade_tick->set_bid_unique_id(raw_trade->m_bid_app_seq_num);
    trade_tick->set_symbol(raw_trade->m_header.m_symbol);
    trade_tick->set_exec_price(to_szse_price(raw_trade->m_exe_px));
    trade_tick->set_exec_quantity(to_szse_quantity(raw_trade->m_exe_qty));
    trade_tick->set_exchange_time(to_szse_time(raw_trade->m_header.m_quote_update_time));
    trade_tick->set_x_ost_szse_exe_type(to_order_type_from_szse(raw_trade->m_exe_type));

    return trade_tick;
}

template<>
inline booker::MdTradePtr CUTCommonData::to_md_trade<SSEHpfL2Snap>(const std::vector<u_char>& message)
{
    auto md_trade = std::make_shared<types::MdTrade>();

    assert(message.size() == sizeof(SSEHpfL2Snap));
    const auto raw_md_trade = reinterpret_cast<const SSEHpfL2Snap*>(message.data());

    md_trade->set_symbol(raw_md_trade->m_symbol_id);
    md_trade->set_price(to_sse_price(raw_md_trade->m_last_price));
    md_trade->set_quantity(to_sse_quantity(raw_md_trade->m_trade_volume));
    md_trade->set_sell_price_10(to_sse_price(raw_md_trade->m_ask_px[0].m_px));

    md_trade->set_sell_price_1(to_sse_price(raw_md_trade->m_ask_px[0].m_px));
    md_trade->set_sell_quantity_1(to_sse_quantity(raw_md_trade->m_ask_px[0].m_qty));
    md_trade->set_sell_price_2(to_sse_price(raw_md_trade->m_ask_px[1].m_px));
    md_trade->set_sell_quantity_2(to_sse_quantity(raw_md_trade->m_ask_px[1].m_qty));
    md_trade->set_sell_price_3(to_sse_price(raw_md_trade->m_ask_px[2].m_px));
    md_trade->set_sell_quantity_3(to_sse_quantity(raw_md_trade->m_ask_px[2].m_qty));
    md_trade->set_sell_price_4(to_sse_price(raw_md_trade->m_ask_px[3].m_px));
    md_trade->set_sell_quantity_4(to_sse_quantity(raw_md_trade->m_ask_px[3].m_qty));
    md_trade->set_sell_price_5(to_sse_price(raw_md_trade->m_ask_px[4].m_px));
    md_trade->set_sell_quantity_5(to_sse_quantity(raw_md_trade->m_ask_px[4].m_qty));
    md_trade->set_sell_price_6(to_sse_price(raw_md_trade->m_ask_px[5].m_px));
    md_trade->set_sell_quantity_6(to_sse_quantity(raw_md_trade->m_ask_px[5].m_qty));
    md_trade->set_sell_price_7(to_sse_price(raw_md_trade->m_ask_px[6].m_px));
    md_trade->set_sell_quantity_7(to_sse_quantity(raw_md_trade->m_ask_px[6].m_qty));
    md_trade->set_sell_price_8(to_sse_price(raw_md_trade->m_ask_px[7].m_px));
    md_trade->set_sell_quantity_8(to_sse_quantity(raw_md_trade->m_ask_px[7].m_qty));
    md_trade->set_sell_price_9(to_sse_price(raw_md_trade->m_ask_px[8].m_px));
    md_trade->set_sell_quantity_9(to_sse_quantity(raw_md_trade->m_ask_px[8].m_qty));
    md_trade->set_sell_price_10(to_sse_price(raw_md_trade->m_ask_px[9].m_px));
    md_trade->set_sell_quantity_10(to_sse_quantity(raw_md_trade->m_ask_px[9].m_qty));

    md_trade->set_buy_price_1(to_sse_price(raw_md_trade->m_bid_px[0].m_px));
    md_trade->set_buy_quantity_1(to_sse_quantity(raw_md_trade->m_bid_px[0].m_qty));
    md_trade->set_buy_price_2(to_sse_price(raw_md_trade->m_bid_px[1].m_px));
    md_trade->set_buy_quantity_2(to_sse_quantity(raw_md_trade->m_bid_px[1].m_qty));
    md_trade->set_buy_price_3(to_sse_price(raw_md_trade->m_bid_px[2].m_px));
    md_trade->set_buy_quantity_3(to_sse_quantity(raw_md_trade->m_bid_px[2].m_qty));
    md_trade->set_buy_price_4(to_sse_price(raw_md_trade->m_bid_px[3].m_px));
    md_trade->set_buy_quantity_4(to_sse_quantity(raw_md_trade->m_bid_px[3].m_qty));
    md_trade->set_buy_price_5(to_sse_price(raw_md_trade->m_bid_px[4].m_px));
    md_trade->set_buy_quantity_5(to_sse_quantity(raw_md_trade->m_bid_px[4].m_qty));
    md_trade->set_buy_price_6(to_sse_price(raw_md_trade->m_bid_px[5].m_px));
    md_trade->set_buy_quantity_6(to_sse_quantity(raw_md_trade->m_bid_px[5].m_qty));
    md_trade->set_buy_price_7(to_sse_price(raw_md_trade->m_bid_px[6].m_px));
    md_trade->set_buy_quantity_7(to_sse_quantity(raw_md_trade->m_bid_px[6].m_qty));
    md_trade->set_buy_price_8(to_sse_price(raw_md_trade->m_bid_px[7].m_px));
    md_trade->set_buy_quantity_8(to_sse_quantity(raw_md_trade->m_bid_px[7].m_qty));
    md_trade->set_buy_price_9(to_sse_price(raw_md_trade->m_bid_px[8].m_px));
    md_trade->set_buy_quantity_9(to_sse_quantity(raw_md_trade->m_bid_px[8].m_qty));
    md_trade->set_buy_price_10(to_sse_price(raw_md_trade->m_bid_px[9].m_px));
    md_trade->set_buy_quantity_10(to_sse_quantity(raw_md_trade->m_bid_px[9].m_qty));

    return md_trade;
}

template<>
inline booker::MdTradePtr CUTCommonData::to_md_trade<SZSEHpfL2Snap>(const std::vector<u_char>& message)
{
    auto md_trade = std::make_shared<types::MdTrade>();

    assert(message.size() == sizeof(SZSEHpfL2Snap));
    const auto raw_md_trade = reinterpret_cast<const SZSEHpfL2Snap*>(message.data());

    md_trade->set_symbol(raw_md_trade->m_header.m_symbol);
    md_trade->set_price(to_szse_price(raw_md_trade->m_last_price));
    md_trade->set_quantity(to_szse_quantity(raw_md_trade->m_total_quantity_trade));

    md_trade->set_sell_price_1(to_szse_price(raw_md_trade->m_ask_unit[0].m_price));
    md_trade->set_sell_quantity_1(to_szse_quantity(raw_md_trade->m_ask_unit[0].m_qty));
    md_trade->set_sell_price_2(to_szse_price(raw_md_trade->m_ask_unit[1].m_price));
    md_trade->set_sell_quantity_2(to_szse_quantity(raw_md_trade->m_ask_unit[1].m_qty));
    md_trade->set_sell_price_3(to_szse_price(raw_md_trade->m_ask_unit[2].m_price));
    md_trade->set_sell_quantity_3(to_szse_quantity(raw_md_trade->m_ask_unit[2].m_qty));
    md_trade->set_sell_price_4(to_szse_price(raw_md_trade->m_ask_unit[3].m_price));
    md_trade->set_sell_quantity_4(to_szse_quantity(raw_md_trade->m_ask_unit[3].m_qty));
    md_trade->set_sell_price_5(to_szse_price(raw_md_trade->m_ask_unit[4].m_price));
    md_trade->set_sell_quantity_5(to_szse_quantity(raw_md_trade->m_ask_unit[4].m_qty));
    md_trade->set_sell_price_6(to_szse_price(raw_md_trade->m_ask_unit[5].m_price));
    md_trade->set_sell_quantity_6(to_szse_quantity(raw_md_trade->m_ask_unit[5].m_qty));
    md_trade->set_sell_price_7(to_szse_price(raw_md_trade->m_ask_unit[6].m_price));
    md_trade->set_sell_quantity_7(to_szse_quantity(raw_md_trade->m_ask_unit[6].m_qty));
    md_trade->set_sell_price_8(to_szse_price(raw_md_trade->m_ask_unit[7].m_price));
    md_trade->set_sell_quantity_8(to_szse_quantity(raw_md_trade->m_ask_unit[7].m_qty));
    md_trade->set_sell_price_9(to_szse_price(raw_md_trade->m_ask_unit[8].m_price));
    md_trade->set_sell_quantity_9(to_szse_quantity(raw_md_trade->m_ask_unit[8].m_qty));
    md_trade->set_sell_price_10(to_szse_price(raw_md_trade->m_ask_unit[9].m_price));
    md_trade->set_sell_quantity_10(to_szse_quantity(raw_md_trade->m_ask_unit[9].m_qty));

    md_trade->set_buy_price_1(to_szse_price(raw_md_trade->m_bid_unit[0].m_price));
    md_trade->set_buy_quantity_1(to_szse_quantity(raw_md_trade->m_bid_unit[0].m_qty));
    md_trade->set_buy_price_2(to_szse_price(raw_md_trade->m_bid_unit[1].m_price));
    md_trade->set_buy_quantity_2(to_szse_quantity(raw_md_trade->m_bid_unit[1].m_qty));
    md_trade->set_buy_price_3(to_szse_price(raw_md_trade->m_bid_unit[2].m_price));
    md_trade->set_buy_quantity_3(to_szse_quantity(raw_md_trade->m_bid_unit[2].m_qty));
    md_trade->set_buy_price_4(to_szse_price(raw_md_trade->m_bid_unit[3].m_price));
    md_trade->set_buy_quantity_4(to_szse_quantity(raw_md_trade->m_bid_unit[3].m_qty));
    md_trade->set_buy_price_5(to_szse_price(raw_md_trade->m_bid_unit[4].m_price));
    md_trade->set_buy_quantity_5(to_szse_quantity(raw_md_trade->m_bid_unit[4].m_qty));
    md_trade->set_buy_price_6(to_szse_price(raw_md_trade->m_bid_unit[5].m_price));
    md_trade->set_buy_quantity_6(to_szse_quantity(raw_md_trade->m_bid_unit[5].m_qty));
    md_trade->set_buy_price_7(to_szse_price(raw_md_trade->m_bid_unit[6].m_price));
    md_trade->set_buy_quantity_7(to_szse_quantity(raw_md_trade->m_bid_unit[6].m_qty));
    md_trade->set_buy_price_8(to_szse_price(raw_md_trade->m_bid_unit[7].m_price));
    md_trade->set_buy_quantity_8(to_szse_quantity(raw_md_trade->m_bid_unit[7].m_qty));
    md_trade->set_buy_price_9(to_szse_price(raw_md_trade->m_bid_unit[8].m_price));
    md_trade->set_buy_quantity_9(to_szse_quantity(raw_md_trade->m_bid_unit[8].m_qty));
    md_trade->set_buy_price_10(to_szse_price(raw_md_trade->m_bid_unit[9].m_price));
    md_trade->set_buy_quantity_10(to_szse_quantity(raw_md_trade->m_bid_unit[9].m_qty));

    return md_trade;
}

} // namespace trade::broker
