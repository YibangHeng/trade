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
    [[nodiscard]] static booker::OrderTickPtr to_order_tick(const std::string& message);
    template<IsTradeTick Mess7ageType>
    [[nodiscard]] static booker::TradeTickPtr to_trade_tick(const std::string& message);
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
inline booker::OrderTickPtr CUTCommonData::to_order_tick<SSEHpfTick>(const std::string& message)
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
inline booker::OrderTickPtr CUTCommonData::to_order_tick<SZSEHpfOrderTick>(const std::string& message)
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
inline booker::TradeTickPtr CUTCommonData::to_trade_tick<SZSEHpfTradeTick>(const std::string& message)
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

} // namespace trade::broker
