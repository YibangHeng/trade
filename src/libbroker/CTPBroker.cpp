#include "libbroker/CTPBroker.h"
#include "utilities/GB2312ToUTF8.hpp"
#include "utilities/MakeAssignable.hpp"

trade::broker::CTPBrokerImpl::CTPBrokerImpl(CTPBroker* parent)
    : AppBase("CTPBrokerImpl", parent->config),
      m_api(CThostFtdcTraderApi::CreateFtdcTraderApi()),
      m_holder(parent->m_holder),
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
    int code, request_seq, request_id;

    std::tie(request_seq, request_id) = new_id_pair();

    /// @OnRspQryExchange.
    CThostFtdcQryExchangeField qry_exchange_field {};
    code = m_api->ReqQryExchange(&qry_exchange_field, request_seq);
    if (code != 0) {
        logger->error("Failed to call ReqQryExchange: returned code {}", code);
    }

    std::tie(request_seq, request_id) = new_id_pair();

    /// @OnReqQryProduct.
    CThostFtdcQryProductField qry_product_field {};
    code = m_api->ReqQryProduct(&qry_product_field, request_seq);
    if (code != 0) {
        logger->error("Failed to call ReqQryProduct: returned code {}", code);
    }

    std::tie(request_seq, request_id) = new_id_pair();

    /// @OnReqQryInstrument.
    CThostFtdcQryInstrumentField qry_instrument_field {};
    code = m_api->ReqQryInstrument(&qry_instrument_field, request_seq);
    if (code != 0) {
        logger->error("Failed to call ReqQryInstrument: returned code {}", code);
    }

    std::tie(request_seq, request_id) = new_id_pair();

    /// @OnReqQryTradingAccount.
    CThostFtdcQryTradingAccountField qry_trading_account_field {};
    code = m_api->ReqQryTradingAccount(&qry_trading_account_field, request_seq);
    if (code != 0) {
        logger->error("Failed to call ReqQryTradingAccount: returned code {}", code);
    }

    std::tie(request_seq, request_id) = new_id_pair();

    /// @OnQryInvestorPosition.
    CThostFtdcQryInvestorPositionField qry_investor_position_field {};
    code = m_api->ReqQryInvestorPosition(&qry_investor_position_field, request_seq);
    if (code != 0) {
        logger->error("Failed to call ReqQryInvestorPosition: returned code {}", code);
    }

    std::tie(request_seq, request_id) = new_id_pair();

    /// @OnReqQryOrder.
    CThostFtdcQryOrderField qry_order_field {};
    code = m_api->ReqQryOrder(&qry_order_field, request_seq);
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

    const auto request_id = get_by_seq_id(nRequestID);

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
        logger->info("Loaded {} exchanges in request {}", m_exchanges.size(), request_id);
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

    const auto request_id = get_by_seq_id(nRequestID);

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
        logger->info("Loaded {} products in request {}", m_products.size(), request_id);
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

    const auto request_id = get_by_seq_id(nRequestID);

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
        logger->info("Loaded {} instruments in request {}", m_instruments.size(), request_id);
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

    const auto request_id = get_by_seq_id(nRequestID);

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
        logger->warn("No trading account loaded in request {}", request_id);
        return;
    }

    m_trading_account.emplace(pTradingAccount->BrokerID, *pTradingAccount);

    logger->debug("Loaded trading account {}/{} with available funds {} {}", pTradingAccount->AccountID, pTradingAccount->BrokerID, pTradingAccount->Available, pTradingAccount->CurrencyID);

    /// For caching between multiple calls.
    static std::unordered_map<decltype(snow_flaker()), types::Funds> fund_cache;

    auto& funds = fund_cache[request_id];

    types::Fund fund;

    fund.set_account_id(pTradingAccount->AccountID);
    fund.set_available_fund(pTradingAccount->Available);
    fund.set_withdrawn_fund(pTradingAccount->Withdraw);
    fund.set_frozen_fund(pTradingAccount->FrozenCash);
    fund.set_frozen_margin(pTradingAccount->FrozenMargin);
    fund.set_frozen_commission(pTradingAccount->FrozenCommission);

    funds.add_funds()->CopyFrom(fund);

    if (bIsLast) {
        logger->info("Loaded {} trading accounts in request {}", m_trading_account.size(), request_id);
        m_holder->init_funds(std::move(funds));
        fund_cache.erase(request_id);
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

    const auto request_id = get_by_seq_id(nRequestID);

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
        logger->warn("No position loaded in request {}", request_id);
        return;
    }

    m_positions.emplace(pInvestorPosition->InstrumentID, *pInvestorPosition);

    logger->debug("Loaded position {} - {}", pInvestorPosition->InstrumentID, pInvestorPosition->Position);

    /// For caching between multiple calls.
    static std::unordered_map<decltype(snow_flaker()), types::Positions> position_cache;

    auto& positions = position_cache[request_id];

    types::Position position;

    position.set_symbol(pInvestorPosition->InstrumentID);
    position.set_yesterday_position(pInvestorPosition->YdPosition);
    position.set_today_position(pInvestorPosition->Position);
    position.set_open_volume(pInvestorPosition->OpenVolume);
    position.set_close_volume(pInvestorPosition->CloseVolume);
    position.set_position_cost(pInvestorPosition->PositionCost);
    position.set_pre_margin(pInvestorPosition->PreMargin);
    position.set_used_margin(pInvestorPosition->UseMargin);
    position.set_frozen_margin(pInvestorPosition->FrozenMargin);
    position.set_open_cost(pInvestorPosition->OpenCost);
    auto update_time = now();
    position.set_allocated_update_time(&update_time);

    positions.add_positions()->CopyFrom(position);

    if (bIsLast) {
        logger->info("Loaded {} positions in request {}", m_positions.size(), request_id);
        m_holder->init_positions(std::move(positions));
        position_cache.erase(request_id);
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

    const auto request_id = get_by_seq_id(nRequestID);

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
        logger->warn("No order loaded in request {}", request_id);
        return;
    }

    m_orders.emplace(pOrder->OrderRef, *pOrder);

    logger->debug("Loaded order {} - {}", pOrder->OrderRef, utilities::GB2312ToUTF8()(pOrder->InstrumentID));

    if (bIsLast) {
        logger->info("Loaded {} orders in request {}", m_orders.size(), request_id);
    }
}

google::protobuf::Timestamp trade::broker::CTPBrokerImpl::now()
{
    google::protobuf::Timestamp timestamp;

    const auto now     = std::chrono::system_clock::now();

    const auto seconds = std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count();
    timestamp.set_seconds(seconds);

    const auto nanoseconds = std::chrono::duration_cast<std::chrono::nanoseconds>(now.time_since_epoch()).count() % 1000000000;
    timestamp.set_nanos(static_cast<int32_t>(nanoseconds));

    return timestamp;
}

trade::broker::CTPBroker::CTPBroker(
    const std::string& config_path,
    const std::shared_ptr<holder::IHolder>& holder
) : BrokerProxy("CTPBroker", holder, config_path),
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
