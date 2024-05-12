#pragma once

#include "AppBase.hpp"
#include "CTPCommonData.h"
#include "libholder/IHolder.h"
#include "libreporter/IReporter.hpp"
#include "third/ctp/ThostFtdcTraderApi.h"
#include "utilities/LoginSyncer.hpp"

namespace trade::broker
{

class CTPTraderImpl final: private AppBase<int>, private CThostFtdcTraderSpi
{
public:
    explicit CTPTraderImpl(
        std::shared_ptr<ConfigType> config,
        std::shared_ptr<holder::IHolder> holder,
        std::shared_ptr<reporter::IReporter> reporter,
        utilities::LoginSyncer* parent
    );
    ~CTPTraderImpl() override;

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
    CThostFtdcTraderApi* m_trader_api;
    CTPCommonData m_common_data;
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
    std::unordered_map<decltype(AppBase<>::ticker_taper()), decltype(NEW_ID())> m_id_map;
    std::shared_ptr<holder::IHolder> m_holder;
    std::shared_ptr<reporter::IReporter> m_reporter;

private:
    utilities::LoginSyncer* m_parent;
};

} // namespace trade::broker
