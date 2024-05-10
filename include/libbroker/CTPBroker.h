#pragma once

#include <memory>

#include "AppBase.hpp"
#include "BrokerProxy.hpp"
#include "third/ctp/ThostFtdcTraderApi.h"

namespace trade::broker
{

class CTPBroker;

class CTPBrokerImpl final: private AppBase<int>, private CThostFtdcTraderSpi
{
public:
    explicit CTPBrokerImpl(CTPBroker* parent);
    ~CTPBrokerImpl() override;

public:
    int64_t new_order(const std::shared_ptr<types::NewOrderReq>& new_order_req);
    int64_t cancel_order(const std::shared_ptr<types::NewCancelReq>& new_cancel_req);

private:
    /// Do some initial queries.
    void init_req();

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
    void OnRspQryInvestor(
        CThostFtdcInvestorField* pInvestor,
        CThostFtdcRspInfoField* pRspInfo,
        int nRequestID, bool bIsLast
    ) override;
    void OnRspQryExchange(
        CThostFtdcExchangeField* pExchange,
        CThostFtdcRspInfoField* pRspInfo,
        int nRequestID, bool bIsLast
    ) override;
    void OnRspQryProduct(
        CThostFtdcProductField* pProduct,
        CThostFtdcRspInfoField* pRspInfo,
        int nRequestID, bool bIsLast
    ) override;
    void OnRspQryInstrument(
        CThostFtdcInstrumentField* pInstrument,
        CThostFtdcRspInfoField* pRspInfo,
        int nRequestID, bool bIsLast
    ) override;
    void OnRspQryTradingAccount(
        CThostFtdcTradingAccountField* pTradingAccount,
        CThostFtdcRspInfoField* pRspInfo,
        int nRequestID, bool bIsLast
    ) override;
    void OnRspQryInvestorPosition(
        CThostFtdcInvestorPositionField* pInvestorPosition,
        CThostFtdcRspInfoField* pRspInfo,
        int nRequestID, bool bIsLast
    ) override;
    void OnRspOrderInsert(
        CThostFtdcInputOrderField* pInputOrder,
        CThostFtdcRspInfoField* pRspInfo,
        int nRequestID, bool bIsLast
    ) override;
    void OnErrRtnOrderInsert(
        CThostFtdcInputOrderField* pInputOrder,
        CThostFtdcRspInfoField* pRspInfo
    ) override;
    void OnRspOrderAction(
        CThostFtdcInputOrderActionField* pInputOrderAction,
        CThostFtdcRspInfoField* pRspInfo,
        int nRequestID, bool bIsLast
    ) override;
    void OnErrRtnOrderAction(
        CThostFtdcOrderActionField* pOrderAction,
        CThostFtdcRspInfoField* pRspInfo
    ) override;
    void OnRtnOrder(CThostFtdcOrderField* pOrder) override;

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
    [[nodiscard]] static auto from_broker_id(const std::string& broker_id);
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
    [[nodiscard]] static auto from_exchange_id(const std::string& exchange_id);
    [[nodiscard]] static google::protobuf::Timestamp* now();

private:
    CThostFtdcTraderApi* m_api;
    TThostFtdcBrokerIDType m_broker_id;
    TThostFtdcUserIDType m_user_id;
    TThostFtdcInvestorIDType m_investor_id;
    TThostFtdcFrontIDType m_front_id;
    TThostFtdcSessionIDType m_session_id;
    /// ExchangeID -> CThostFtdcExchangeField.
    std::unordered_map<std::string, CThostFtdcExchangeField> m_exchanges;
    /// ProductID -> CThostFtdcProductField.
    std::unordered_map<std::string, CThostFtdcProductField> m_products;
    /// InstrumentID -> CThostFtdcInstrumentField.
    std::unordered_map<std::string, CThostFtdcInstrumentField> m_instruments;
    /// InstrumentID -> CThostFtdcTradingAccountField.
    std::unordered_map<std::string, CThostFtdcTradingAccountField> m_trading_account;
    /// InstrumentID -> CThostFtdcInvestorPositionField.
    std::unordered_map<std::string, CThostFtdcInvestorPositionField> m_positions;
    /// OrderRef -> CThostFtdcOrderField.
    std::unordered_map<std::string, CThostFtdcOrderField> m_orders;

private:
    /// nRequestID -> UniqueID/RequestID.
    std::unordered_map<decltype(ticker_taper()), decltype(NEW_ID())> m_id_map;
    std::shared_ptr<holder::IHolder> m_holder;
    std::shared_ptr<reporter::IReporter> m_reporter;

private:
    CTPBroker* m_parent;
};

class PUBLIC_API CTPBroker final: public BrokerProxy<int>
{
    friend class CTPBrokerImpl;

public:
    explicit CTPBroker(
        const std::string& config_path,
        const std::shared_ptr<holder::IHolder>& holder,
        const std::shared_ptr<reporter::IReporter>& reporter
    );
    ~CTPBroker() override = default;

public:
    void login() noexcept override;
    void logout() noexcept override;

public:
    int64_t new_order(std::shared_ptr<types::NewOrderReq> new_order_req) override;
    int64_t cancel_order(std::shared_ptr<types::NewCancelReq> new_cancel_req) override;

private:
    std::unique_ptr<CTPBrokerImpl> m_impl;
};

} // namespace trade::broker
