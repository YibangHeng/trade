#pragma once

#include "AppBase.hpp"
#include "CUTCommonData.h"
#include "libholder/IHolder.h"
#include "libreporter/IReporter.hpp"
#include "third/cut/UTApi.h"
#include "utilities/LoginSyncer.hpp"

namespace trade::broker
{

class CUTTraderImpl final: private AppBase<int>, private CUTSpi
{
public:
    explicit CUTTraderImpl(
        std::shared_ptr<ConfigType> config,
        std::shared_ptr<holder::IHolder> holder,
        std::shared_ptr<reporter::IReporter> reporter,
        utilities::LoginSyncer* parent
    );
    ~CUTTraderImpl() override;

public:
    void new_order(
        const std::shared_ptr<types::NewOrderReq>& new_order_req,
        const std::shared_ptr<types::NewOrderRsp>& new_order_rsp
    );
    void cancel_order(
        const std::shared_ptr<types::NewCancelReq>& new_cancel_req,
        const std::shared_ptr<types::NewCancelRsp>& new_cancel_rsp
    );

private:
    /// Do some initial queries.
    void init_req();

private:
    void OnFrontConnected() override;
    void OnRspLogin(
        CUTRspLoginField* pRspLogin,
        CUTRspInfoField* pRspInfo,
        int nRequestID, bool bIsLast
    ) override;
    void OnFrontDisconnected(int nReason) override;
    void OnRspQryInvestor(
        CUTInvestorField* pInvestor,
        CUTRspInfoField* pRspInfo,
        int nRequestID, bool bIsLast
    ) override;
    void OnRspQryInstrument(
        CUTInstrumentField* pInstrument,
        CUTRspInfoField* pRspInfo,
        int nRequestID, bool bIsLast
    ) override;
    void OnRspQryTradingAccount(
        CUTTradingAccountField* pTradingAccount,
        CUTRspInfoField* pRspInfo,
        int nRequestID, bool bIsLast
    ) override;
    void OnRspQryInvestorPosition(
        CUTInvestorPositionField* pInvestorPosition,
        CUTRspInfoField* pRspInfo,
        int nRequestID, bool bIsLast
    ) override;
    void OnRspOrderInsert(
        CUTInputOrderField* pInputOrder,
        CUTRspInfoField* pRspInfo,
        int nRequestID, bool bIsLast
    ) override;
    void OnRspOrderAction(
        CUTInputOrderActionField* pInputOrderAction,
        CUTRspInfoField* pRspInfo,
        int nRequestID, bool bIsLast
    ) override;
    void OnErrRtnOrderAction(CUTOrderActionField* pOrderAction) override;
    void OnRtnOrder(CUTOrderField* pOrder) override;

private:
    CUTApi* m_trader_api;
    CUTCommonData m_common_data;

private:
    /// nRequestID -> UniqueID/RequestID.
    std::unordered_map<decltype(AppBase<>::ticker_taper()), decltype(NEW_ID())> m_id_map;
    std::shared_ptr<holder::IHolder> m_holder;
    std::shared_ptr<reporter::IReporter> m_reporter;

private:
    utilities::LoginSyncer* m_parent;
};

} // namespace trade::broker
