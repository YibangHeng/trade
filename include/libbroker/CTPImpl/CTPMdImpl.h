#pragma once

#include "AppBase.hpp"
#include "libholder/IHolder.h"
#include "libreporter/IReporter.hpp"
#include "third/ctp/ThostFtdcMdApi.h"
#include "utilities/LoginSyncer.hpp"

namespace trade::broker
{

class CTPMdImpl final: private AppBase<int>, private CThostFtdcMdSpi
{
public:
    explicit CTPMdImpl(
        std::shared_ptr<ConfigType> config,
        std::shared_ptr<holder::IHolder> holder,
        std::shared_ptr<reporter::IReporter> reporter
    );
    ~CTPMdImpl() override;

private:
    void OnFrontConnected() override;
    void OnRspUserLogin(
        CThostFtdcRspUserLoginField* pRspUserLogin,
        CThostFtdcRspInfoField* pRspInfo,
        int nRequestID, bool bIsLast
    ) override;
    void OnRspUserLogout(
        CThostFtdcUserLogoutField* pUserLogout,
        CThostFtdcRspInfoField* pRspInfo,
        int nRequestID, bool bIsLast
    ) override;

private:
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

private:
    CThostFtdcMdApi* m_md_api;
    TThostFtdcBrokerIDType m_broker_id;
    TThostFtdcUserIDType m_user_id;
    TThostFtdcInvestorIDType m_investor_id;
    TThostFtdcFrontIDType m_front_id;
    TThostFtdcSessionIDType m_session_id;

private:
    /// nRequestID -> UniqueID/RequestID.
    std::unordered_map<decltype(AppBase<>::ticker_taper()), decltype(NEW_ID())> m_id_map;
    std::shared_ptr<holder::IHolder> m_holder;
    std::shared_ptr<reporter::IReporter> m_reporter;

private:
    /// Use self-managed login syncer for keeping trade running without market
    /// data.
    /// m_login_syncer always notifies success even if login fails.
    utilities::LoginSyncer m_login_syncer;
};

} // namespace trade::broker
