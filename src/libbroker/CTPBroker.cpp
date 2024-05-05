#include "libbroker/CTPBroker.h"
#include "utilities/MakeAssignable.hpp"

trade::broker::CTPBrokerImpl::CTPBrokerImpl(CTPBroker* parent)
    : AppBase("CTPBrokerImpl", parent->config),
      m_api(CThostFtdcTraderApi::CreateFtdcTraderApi()),
      m_parent(parent)
{
    m_api->RegisterSpi(this);
    m_api->RegisterFront(const_cast<char*>(config->get<std::string>("Server.TradeAddress").c_str()));

    m_api->SubscribePublicTopic(THOST_TERT_RESTART);
    m_api->SubscribePrivateTopic(THOST_TERT_RESTART);

    m_api->Init();
}

trade::broker::CTPBrokerImpl::~CTPBrokerImpl()
{
    CThostFtdcUserLogoutField req {};

    M_A {req.BrokerID} = config->get<std::string>("User.BrokerID");
    M_A {req.UserID}   = config->get<std::string>("User.UserID");

    m_api->ReqUserLogout(&req, ticker_taper());

    /// Wait for @OnRspUserLogout.
    m_parent->wait_logout();

    m_api->RegisterSpi(nullptr);
    m_api->Release();
}

void trade::broker::CTPBrokerImpl::OnFrontConnected()
{
    CThostFtdcTraderSpi::OnFrontConnected();

    logger->info("Connected to CTP server {} with version {}", config->get<std::string>("Server.TradeAddress"), CThostFtdcTraderApi::GetApiVersion());

    CThostFtdcReqUserLoginField req {};

    M_A {req.BrokerID} = config->get<std::string>("User.BrokerID");
    M_A {req.UserID}   = config->get<std::string>("User.UserID");
    M_A {req.Password} = config->get<std::string>("User.Password");

    /// Req user login.
    const auto code = m_api->ReqUserLogin(&req, ticker_taper());
    if (code != 0) {
        m_parent->notify_login_failure("Failed to call ReqUserLogin with code {}", code);
    }
}

void trade::broker::CTPBrokerImpl::OnRspUserLogin(
    CThostFtdcRspUserLoginField* pRspUserLogin,
    CThostFtdcRspInfoField* pRspInfo,
    const int nRequestID, const bool bIsLast
)
{
    CThostFtdcTraderSpi::OnRspUserLogin(pRspUserLogin, pRspInfo, nRequestID, bIsLast);

    if (pRspInfo == nullptr) {
        return;
    }

    if (pRspInfo->ErrorID != 0) {
        m_parent->notify_login_failure("Failed to login with code {}: {}", pRspInfo->ErrorID, pRspInfo->ErrorMsg);
        return;
    }

    logger->info("Logged to {} successfully as UserID/BrokerID {}/{} at {}-{}", pRspUserLogin->SystemName, pRspUserLogin->UserID, pRspUserLogin->BrokerID, pRspUserLogin->TradingDay, pRspUserLogin->LoginTime);

    m_parent->notify_login_success();
}

void trade::broker::CTPBrokerImpl::OnRspUserLogout(
    CThostFtdcUserLogoutField* pUserLogout,
    CThostFtdcRspInfoField* pRspInfo,
    const int nRequestID, const bool bIsLast
)
{
    CThostFtdcTraderSpi::OnRspUserLogout(pUserLogout, pRspInfo, nRequestID, bIsLast);

    if (pRspInfo == nullptr) {
        return;
    }

    if (pRspInfo->ErrorID != 0) {
        m_parent->notify_logout_failure("Failed to logout with code {}: {}", pRspInfo->ErrorID, pRspInfo->ErrorMsg);
        return;
    }

    logger->info("Logged out successfully as UserID/BrokerID {}/{}", pUserLogout->BrokerID, pUserLogout->UserID);

    m_parent->notify_logout_success();
}

trade::broker::CTPBroker::CTPBroker(const std::string& config_path)
    : BrokerProxy("CTPBroker", config_path),
      m_impl(nullptr)
{
}

void trade::broker::CTPBroker::login() noexcept
{
    BrokerProxy::login();

    m_impl = std::make_unique<CTPBrokerImpl>(this);
}

void trade::broker::CTPBroker::logout() noexcept
{
    BrokerProxy::logout();

    m_impl = nullptr;
}
