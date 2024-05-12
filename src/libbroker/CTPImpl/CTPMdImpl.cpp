#include <google/protobuf/util/time_util.h>
#include <regex>
#include <utility>
#include <vector>

#include "libbroker/CTPImpl/CTPMdImpl.h"
#include "utilities/GB2312ToUTF8.hpp"
#include "utilities/MakeAssignable.hpp"
#include "utilities/TimeHelper.hpp"

trade::broker::CTPMdImpl::CTPMdImpl(
    std::shared_ptr<ConfigType> config,
    std::shared_ptr<holder::IHolder> holder,
    std::shared_ptr<reporter::IReporter> reporter
)
    : AppBase("CTPMdImpl", std::move(config)),
      m_md_api(CThostFtdcMdApi::CreateFtdcMdApi()),
      m_holder(std::move(holder)),
      m_reporter(std::move(reporter))
{
    m_md_api->RegisterSpi(this);
    m_md_api->RegisterFront(const_cast<char*>(AppBase::config->get<std::string>("Server.MdAddress").c_str()));

    m_md_api->Init();
}

trade::broker::CTPMdImpl::~CTPMdImpl()
{
    CThostFtdcUserLogoutField user_logout_field {};

    M_A {user_logout_field.BrokerID} = m_common_data.m_broker_id;
    M_A {user_logout_field.UserID}   = m_common_data.m_user_id;

    m_login_syncer.start_logout();

    m_md_api->ReqUserLogout(&user_logout_field, ticker_taper());

    /// Wait for @OnRspUserLogout.
    m_login_syncer.wait_logout();

    m_md_api->RegisterSpi(nullptr);
    m_md_api->Release();
}

void trade::broker::CTPMdImpl::subscribe(const std::unordered_set<std::string>& symbols) const
{
    std::vector<char*> symbol_vector;
    for (const auto& symbol : symbols) {
        const auto symbol_c_array = new char[symbol.size() + 1];
        std::strcpy(symbol_c_array, symbol.c_str());
        symbol_vector.push_back(symbol_c_array);
    }

    m_md_api->SubscribeMarketData(symbol_vector.data(), static_cast<int>(symbol_vector.size()));

    for (const auto& cstr : symbol_vector) {
        delete[] cstr;
    }
}

void trade::broker::CTPMdImpl::unsubscribe(const std::unordered_set<std::string>& symbols) const
{
    std::vector<char*> symbol_vector;
    for (const auto& symbol : symbols) {
        const auto symbol_c_array = new char[symbol.size() + 1];
        std::strcpy(symbol_c_array, symbol.c_str());
        symbol_vector.push_back(symbol_c_array);
    }

    m_md_api->UnSubscribeMarketData(symbol_vector.data(), static_cast<int>(symbol_vector.size()));

    for (const auto& cstr : symbol_vector) {
        delete[] cstr;
    }
}

#define NULLPTR_CHECKER(ptr)                                      \
    if (ptr == nullptr) {                                         \
        logger->warn("A nullptr {} found in {}", #ptr, __func__); \
        return;                                                   \
    }

void trade::broker::CTPMdImpl::OnFrontConnected()
{
    CThostFtdcMdSpi::OnFrontConnected();

    logger->info("Connected to CTP Md server {} with version {}", config->get<std::string>("Server.TradeAddress"), CThostFtdcMdApi::GetApiVersion());

    CThostFtdcReqUserLoginField req_user_login_field {};

    M_A {req_user_login_field.BrokerID} = config->get<std::string>("User.BrokerID");
    M_A {req_user_login_field.UserID}   = config->get<std::string>("User.UserID");
    M_A {req_user_login_field.Password} = config->get<std::string>("User.Password");

    /// @OnRspUserLogin.
    const auto code = m_md_api->ReqUserLogin(&req_user_login_field, ticker_taper());
    if (code != 0) {
        logger->error("Failed to call ReqUserLogin: returned code {}", code);
    }
}

void trade::broker::CTPMdImpl::OnRspUserLogin(
    CThostFtdcRspUserLoginField* pRspUserLogin,
    CThostFtdcRspInfoField* pRspInfo,
    const int nRequestID, const bool bIsLast
)
{
    CThostFtdcMdSpi::OnRspUserLogin(pRspUserLogin, pRspInfo, nRequestID, bIsLast);

    NULLPTR_CHECKER(pRspInfo);

    if (pRspInfo->ErrorID != 0) {
        logger->error("Failed to login with code {}: {}", pRspInfo->ErrorID, utilities::GB2312ToUTF8()(pRspInfo->ErrorMsg));
    }
    else {
        M_A {m_common_data.m_broker_id} = pRspUserLogin->BrokerID;
        M_A {m_common_data.m_user_id}   = pRspUserLogin->UserID;
        m_common_data.m_front_id        = pRspUserLogin->FrontID;
        m_common_data.m_session_id      = pRspUserLogin->SessionID;

        logger->info("Logged to {} successfully as BrokerID/UserID {}/{} with FrontID/SessionID {}/{} at {}-{}", pRspUserLogin->SystemName, pRspUserLogin->BrokerID, pRspUserLogin->UserID, pRspUserLogin->FrontID, pRspUserLogin->SessionID, pRspUserLogin->TradingDay, pRspUserLogin->LoginTime);
    }

    m_login_syncer.notify_login_success();
}

void trade::broker::CTPMdImpl::OnRspUserLogout(
    CThostFtdcUserLogoutField* pUserLogout,
    CThostFtdcRspInfoField* pRspInfo,
    const int nRequestID, const bool bIsLast
)
{
    CThostFtdcMdSpi::OnRspUserLogout(pUserLogout, pRspInfo, nRequestID, bIsLast);

    NULLPTR_CHECKER(pRspInfo);

    if (pRspInfo->ErrorID != 0) {
        logger->error("Failed to logout with code {}: {}", pRspInfo->ErrorID, pRspInfo->ErrorMsg);
    }
    else {
        logger->info("Logged out successfully as BrokerID/UserID {}/{} with FrontID/SessionID {}/{}", pUserLogout->BrokerID, pUserLogout->UserID, m_common_data.m_front_id, m_common_data.m_session_id);
    }

    m_login_syncer.notify_logout_success();
}

void trade::broker::CTPMdImpl::OnRspSubMarketData(
    CThostFtdcSpecificInstrumentField* pSpecificInstrument,
    CThostFtdcRspInfoField* pRspInfo,
    const int nRequestID, const bool bIsLast
)
{
    CThostFtdcMdSpi::OnRspSubMarketData(pSpecificInstrument, pRspInfo, nRequestID, bIsLast);

    NULLPTR_CHECKER(pRspInfo);

    if (pRspInfo->ErrorID != 0) {
        logger->error("Failed to subscribe to {} with code {}: {}", pSpecificInstrument->InstrumentID, pRspInfo->ErrorID, pRspInfo->ErrorMsg);
    }
    else {
        logger->info("Subscribed to {} successfully", pSpecificInstrument->InstrumentID);
    }
}

void trade::broker::CTPMdImpl::OnRspUnSubMarketData(
    CThostFtdcSpecificInstrumentField* pSpecificInstrument,
    CThostFtdcRspInfoField* pRspInfo,
    const int nRequestID, const bool bIsLast
)
{
    CThostFtdcMdSpi::OnRspUnSubMarketData(pSpecificInstrument, pRspInfo, nRequestID, bIsLast);

    NULLPTR_CHECKER(pRspInfo);

    if (pRspInfo->ErrorID != 0) {
        logger->error("Failed to unsubscribe from {} with code {}: {}", pSpecificInstrument->InstrumentID, pRspInfo->ErrorID, pRspInfo->ErrorMsg);
    }
    else {
        logger->info("Unsubscribed from {} successfully", pSpecificInstrument->InstrumentID);
    }
}

void trade::broker::CTPMdImpl::OnRtnDepthMarketData(CThostFtdcDepthMarketDataField* pDepthMarketData)
{
    CThostFtdcMdSpi::OnRtnDepthMarketData(pDepthMarketData);

    NULLPTR_CHECKER(pDepthMarketData);

    logger->debug("Received market data for {} with BidPrice1: {}, AskPrice1: {} at {}.{:0>3}", pDepthMarketData->InstrumentID, pDepthMarketData->BidPrice1, pDepthMarketData->AskPrice1, pDepthMarketData->UpdateTime, pDepthMarketData->UpdateMillisec);
}
