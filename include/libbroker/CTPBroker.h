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
    void OnRspQryOrder(
        CThostFtdcOrderField* pOrder,
        CThostFtdcRspInfoField* pRspInfo,
        int nRequestID, bool bIsLast
    ) override;
    void OnRspOrderInsert(
        CThostFtdcInputOrderField* pInputOrder,
        CThostFtdcRspInfoField* pRspInfo,
        int nRequestID, bool bIsLast
    ) override;

private:
    static std::string to_exchange(types::ExchangeType exchange);
    static TThostFtdcDirectionType to_side(types::SideType side);
    static char to_position_side(types::PositionSideType position_side);
    static google::protobuf::Timestamp now();

private:
    CThostFtdcTraderApi* m_api;
    TThostFtdcBrokerIDType m_broker_id;
    TThostFtdcUserIDType m_user_id;
    TThostFtdcInvestorIDType m_investor_id;
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

private:
    CTPBroker* m_parent;
};

class PUBLIC_API CTPBroker final: public BrokerProxy<int>
{
    friend class CTPBrokerImpl;

public:
    explicit CTPBroker(
        const std::string& config_path,
        const std::shared_ptr<holder::IHolder>& holder
    );
    ~CTPBroker() override = default;

public:
    void login() noexcept override;
    void logout() noexcept override;

public:
    int64_t new_order(std::shared_ptr<types::NewOrderReq> new_order_req) override;

private:
    std::unique_ptr<CTPBrokerImpl> m_impl;
};

} // namespace trade::broker
