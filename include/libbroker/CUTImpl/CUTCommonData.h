#pragma once

#include "libbooker/BookerCommonData.h"
#include "networks.pb.h"
#include "third/cut/UTApiStruct.h"

namespace trade::broker
{

class CUTCommonData
{
public:
    CUTCommonData();
    ~CUTCommonData() = default;

public:
    [[nodiscard]] static TUTExchangeIDType to_exchange(types::ExchangeType exchange);
    [[nodiscard]] static types::ExchangeType to_exchange(TUTExchangeIDType exchange);
    [[nodiscard]] static types::OrderType to_order_type(char order_type);
    [[nodiscard]] static TUTDirectionType to_side(types::SideType side);
    [[nodiscard]] static types::SideType to_side(TUTDirectionType side);
    [[nodiscard]] static types::SideType to_md_side(TUTDirectionType side);
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
    [[nodiscard]] static types::X_OST_SZSEDatagramType get_datagram_type(const std::string& message);
    [[nodiscard]] static uint8_t to_szse_datagram_type(types::X_OST_SZSEDatagramType message_type);
    [[nodiscard]] static types::X_OST_SZSEDatagramType to_szse_datagram_type(uint8_t message_type);
    [[nodiscard]] static booker::OrderTickPtr to_order_tick(const std::string& message);
    [[nodiscard]] static booker::TradeTickPtr to_trade_tick(const std::string& message);

public:
    TUTSystemNameType m_system_name;
    TUTUserIDType m_user_id;
    TUTInvestorIDType m_investor_id;
    TUTFrontIDType m_front_id;
    TUTSessionIDType m_session_id;
};

} // namespace trade::broker
