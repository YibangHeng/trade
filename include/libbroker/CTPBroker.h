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

private:
    static google::protobuf::Timestamp now();

private:
    CThostFtdcTraderApi* m_api;
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

private:
    std::unique_ptr<CTPBrokerImpl> m_impl;
};

} // namespace trade::broker
