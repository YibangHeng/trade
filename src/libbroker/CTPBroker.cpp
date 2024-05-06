#include "libbroker/CTPBroker.h"
#include "utilities/GB2312ToUTF8.hpp"
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
    CThostFtdcUserLogoutField user_logout_field {};

    M_A {user_logout_field.BrokerID} = config->get<std::string>("User.BrokerID");
    M_A {user_logout_field.UserID}   = config->get<std::string>("User.UserID");

    m_api->ReqUserLogout(&user_logout_field, ticker_taper());

    /// Wait for @OnRspUserLogout.
    m_parent->wait_logout();

    m_api->RegisterSpi(nullptr);
    m_api->Release();
}

void trade::broker::CTPBrokerImpl::init_req()
{
    int code;

    /// @OnRspQryExchange.
    CThostFtdcQryExchangeField qry_exchange_field {};
    code = m_api->ReqQryExchange(&qry_exchange_field, ticker_taper());
    if (code != 0) {
        logger->error("Failed to call ReqQryExchange: returned code {}", code);
    }

    /// @OnReqQryProduct.
    CThostFtdcQryProductField qry_product_field {};
    code = m_api->ReqQryProduct(&qry_product_field, ticker_taper());
    if (code != 0) {
        logger->error("Failed to call ReqQryProduct: returned code {}", code);
    }

    /// @OnReqQryInstrument.
    CThostFtdcQryInstrumentField qry_instrument_field {};
    code = m_api->ReqQryInstrument(&qry_instrument_field, ticker_taper());
    if (code != 0) {
        logger->error("Failed to call ReqQryInstrument: returned code {}", code);
    }

    /// @OnReqQryTradingAccount.
    CThostFtdcQryTradingAccountField qry_trading_account_field {};
    code = m_api->ReqQryTradingAccount(&qry_trading_account_field, ticker_taper());
    if (code != 0) {
        logger->error("Failed to call ReqQryTradingAccount: returned code {}", code);
    }

    /// @OnQryInvestorPosition.
    CThostFtdcQryInvestorPositionField qry_investor_position_field {};
    code = m_api->ReqQryInvestorPosition(&qry_investor_position_field, ticker_taper());
    if (code != 0) {
        logger->error("Failed to call ReqQryInvestorPosition: returned code {}", code);
    }

    /// @OnReqQryOrder.
    CThostFtdcQryOrderField qry_order_field {};
    code = m_api->ReqQryOrder(&qry_order_field, ticker_taper());
    if (code != 0) {
        logger->error("Failed to call ReqQryOrder: returned code {}", code);
    }
}

void trade::broker::CTPBrokerImpl::OnFrontConnected()
{
    CThostFtdcTraderSpi::OnFrontConnected();

    logger->info("Connected to CTP server {} with version {}", config->get<std::string>("Server.TradeAddress"), CThostFtdcTraderApi::GetApiVersion());

    CThostFtdcReqUserLoginField req_user_login_field {};

    M_A {req_user_login_field.BrokerID} = config->get<std::string>("User.BrokerID");
    M_A {req_user_login_field.UserID}   = config->get<std::string>("User.UserID");
    M_A {req_user_login_field.Password} = config->get<std::string>("User.Password");

    /// @OnRspUserLogin.
    const auto code = m_api->ReqUserLogin(&req_user_login_field, ticker_taper());
    if (code != 0) {
        m_parent->notify_login_failure("Failed to call ReqUserLogin: returned code {}", code);
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
        m_parent->notify_login_failure("Failed to login with code {}: {}", pRspInfo->ErrorID, utilities::GB2312ToUTF8()(pRspInfo->ErrorMsg));
        return;
    }

    logger->info("Logged to {} successfully as UserID/BrokerID {}/{} at {}-{}", pRspUserLogin->SystemName, pRspUserLogin->UserID, pRspUserLogin->BrokerID, pRspUserLogin->TradingDay, pRspUserLogin->LoginTime);

    m_parent->notify_login_success();

    /// Initial queries.
    init_req();
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

    logger->info("Logged out successfully as UserID/BrokerID {}/{}", pUserLogout->UserID, pUserLogout->BrokerID);

    m_parent->notify_logout_success();
}

/// @ReqQryExchange.
void trade::broker::CTPBrokerImpl::OnRspQryExchange(
    CThostFtdcExchangeField* pExchange,
    CThostFtdcRspInfoField* pRspInfo,
    const int nRequestID, const bool bIsLast
)
{
    CThostFtdcTraderSpi::OnRspQryExchange(pExchange, pRspInfo, nRequestID, bIsLast);

    if (pRspInfo == nullptr) {
        return;
    }

    if (pRspInfo->ErrorID != 0) {
        logger->error("Failed to load exchange: {}", pRspInfo->ErrorMsg);
        return;
    }

    m_exchanges.emplace(pExchange->ExchangeID, *pExchange);

    logger->debug("Loaded exchange {} - {}", pExchange->ExchangeID, utilities::GB2312ToUTF8()(pExchange->ExchangeName));

    if (bIsLast) {
        logger->info("Loaded {} exchanges", m_exchanges.size());
    }
}

/// @ReqQryProduct.
void trade::broker::CTPBrokerImpl::OnRspQryProduct(
    CThostFtdcProductField* pProduct,
    CThostFtdcRspInfoField* pRspInfo,
    const int nRequestID, const bool bIsLast
)
{
    CThostFtdcTraderSpi::OnRspQryProduct(pProduct, pRspInfo, nRequestID, bIsLast);

    if (pRspInfo == nullptr) {
        return;
    }

    if (pRspInfo->ErrorID != 0) {
        logger->error("Failed to load product: {}", pRspInfo->ErrorMsg);
        return;
    }

    m_products.emplace(pProduct->ProductID, *pProduct);

    logger->debug("Loaded product {} - {}", pProduct->ProductID, utilities::GB2312ToUTF8()(pProduct->ProductName));

    if (bIsLast) {
        logger->info("Loaded {} products", m_products.size());
    }
}

/// @ReqQryInstrument.
void trade::broker::CTPBrokerImpl::OnRspQryInstrument(
    CThostFtdcInstrumentField* pInstrument,
    CThostFtdcRspInfoField* pRspInfo,
    const int nRequestID, const bool bIsLast
)
{
    CThostFtdcTraderSpi::OnRspQryInstrument(pInstrument, pRspInfo, nRequestID, bIsLast);

    if (pRspInfo == nullptr) {
        return;
    }

    if (pRspInfo->ErrorID != 0) {
        logger->error("Failed to load instrument: {}", pRspInfo->ErrorMsg);
        return;
    }

    m_instruments.emplace(pInstrument->InstrumentID, *pInstrument);

    logger->debug("Loaded instrument {} - {}", pInstrument->InstrumentID, utilities::GB2312ToUTF8()(pInstrument->InstrumentName));

    if (bIsLast) {
        logger->info("Loaded {} instruments", m_instruments.size());
    }
}

/// @ReqQryTradingAccount.
void trade::broker::CTPBrokerImpl::OnRspQryTradingAccount(
    CThostFtdcTradingAccountField* pTradingAccount,
    CThostFtdcRspInfoField* pRspInfo,
    const int nRequestID, const bool bIsLast
)
{
    CThostFtdcTraderSpi::OnRspQryTradingAccount(pTradingAccount, pRspInfo, nRequestID, bIsLast);

    if (pRspInfo == nullptr) {
        return;
    }

    if (pRspInfo->ErrorID != 0) {
        logger->error("Failed to load trading account: {}", pRspInfo->ErrorMsg);
        return;
    }

    /// pTradingAccount will be nullptr if there is no trading account.
    if (pTradingAccount == nullptr) {
        assert(bIsLast);
        logger->warn("No trading account loaded");
        return;
    }

    m_trading_account.emplace(pTradingAccount->BrokerID, *pTradingAccount);

    logger->debug("Loaded trading account {}/{} with available funds {} {}", pTradingAccount->AccountID, pTradingAccount->BrokerID, pTradingAccount->Available, pTradingAccount->CurrencyID);

    if (bIsLast) {
        logger->info("Loaded {} trading accounts", m_trading_account.size());
    }
}

/// @ReqQryInvestorPosition.
void trade::broker::CTPBrokerImpl::OnRspQryInvestorPosition(
    CThostFtdcInvestorPositionField* pInvestorPosition,
    CThostFtdcRspInfoField* pRspInfo,
    const int nRequestID, const bool bIsLast
)
{
    CThostFtdcTraderSpi::OnRspQryInvestorPosition(pInvestorPosition, pRspInfo, nRequestID, bIsLast);

    if (pRspInfo == nullptr) {
        return;
    }

    if (pRspInfo->ErrorID != 0) {
        logger->error("Failed to load position: {}", pRspInfo->ErrorMsg);
        return;
    }

    /// pInvestorPosition will be nullptr if there is no position.
    if (pInvestorPosition == nullptr) {
        assert(bIsLast);
        logger->warn("No position loaded");
        return;
    }

    m_positions.emplace(pInvestorPosition->InstrumentID, *pInvestorPosition);

    logger->debug("Loaded position {} - {}", pInvestorPosition->InstrumentID, pInvestorPosition->Position);

    if (bIsLast) {
        logger->info("Loaded {} positions", m_positions.size());
    }
}

/// @ReqQryOrder.
void trade::broker::CTPBrokerImpl::OnRspQryOrder(
    CThostFtdcOrderField* pOrder,
    CThostFtdcRspInfoField* pRspInfo,
    const int nRequestID, const bool bIsLast
)
{
    CThostFtdcTraderSpi::OnRspQryOrder(pOrder, pRspInfo, nRequestID, bIsLast);

    if (pRspInfo == nullptr) {
        return;
    }

    if (pRspInfo->ErrorID != 0) {
        logger->error("Failed to load order: {}", pRspInfo->ErrorMsg);
        return;
    }

    /// pOrder will be nullptr if there is no order.
    if (pOrder == nullptr) {
        assert(bIsLast);
        logger->warn("No order loaded");
        return;
    }

    m_orders.emplace(pOrder->OrderRef, *pOrder);

    logger->debug("Loaded order {} - {}", pOrder->OrderRef, utilities::GB2312ToUTF8()(pOrder->InstrumentID));

    if (bIsLast) {
        logger->info("Loaded {} orders", m_orders.size());
    }
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
