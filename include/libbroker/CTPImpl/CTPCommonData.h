#pragma once

#include "networks.pb.h"
#include "third/ctp/ThostFtdcUserApiDataType.h"

namespace trade::broker
{

class CTPCommonData
{
public:
    CTPCommonData();
    ~CTPCommonData() = default;

public:
    [[nodiscard]] static std::string to_exchange(types::ExchangeType exchange);
    [[nodiscard]] static types::ExchangeType to_exchange(const std::string& exchange);
    [[nodiscard]] static TThostFtdcDirectionType to_side(types::SideType side);
    [[nodiscard]] static types::SideType to_side(TThostFtdcDirectionType side);
    [[nodiscard]] static char to_position_side(types::PositionSideType position_side);
    [[nodiscard]] static types::PositionSideType to_position_side(char position_side);
    /// Concatenate front_id, session_id and order_ref to broker_id in format of
    /// {front_id}:{session_id}:{order_ref}.
    /// This tuples uniquely identify a CTP order.
    [[nodiscard]] static std::string to_broker_id(
        TThostFtdcFrontIDType front_id,
        TThostFtdcSessionIDType session_id,
        const std::string& order_ref
    );
    /// Extract front_id, session_id and order_ref from broker_id in format of
    /// {front_id}:{session_id}:{order_ref}.
    /// @return std::tuple<front_id, session_id, order_ref>. Empty string if
    /// broker_id is not in format.
    [[nodiscard]] static std::tuple<std::string, std::string, std::string> from_broker_id(const std::string& broker_id);
    /// Concatenate exchange and order_sys_id to exchange_id in format of
    /// {exchange}:{order_sys_id}.
    /// This tuples uniquely identify a CTP order.
    [[nodiscard]] static std::string to_exchange_id(
        const std::string& exchange,
        const std::string& order_sys_id
    );
    /// Extract exchange and order_sys_id from exchange_id in format of
    /// {exchange}:{order_sys_id}.
    /// @return std::tuple<exchange, order_sys_id>. Empty string if exchange_id
    /// is not in format.
    [[nodiscard]] static std::tuple<std::string, std::string> from_exchange_id(const std::string& exchange_id);

public:
    TThostFtdcBrokerIDType m_broker_id;
    TThostFtdcUserIDType m_user_id;
    TThostFtdcInvestorIDType m_investor_id;
    TThostFtdcFrontIDType m_front_id;
    TThostFtdcSessionIDType m_session_id;
};

} // namespace trade::broker
