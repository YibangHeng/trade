#include <google/protobuf/util/time_util.h>

#include "libbroker/CTPBroker.h"
#include "utilities/GB2312ToUTF8.hpp"
#include "utilities/MakeAssignable.hpp"

trade::broker::CTPBrokerImpl::CTPBrokerImpl(CTPBroker* parent)
    : AppBase("CTPBrokerImpl", parent->config),
      m_api(CThostFtdcTraderApi::CreateFtdcTraderApi()),
      m_broker_id(),
      m_user_id(),
      m_investor_id(),
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

    M_A {user_logout_field.BrokerID} = m_broker_id;
    M_A {user_logout_field.UserID}   = m_user_id;

    m_api->ReqUserLogout(&user_logout_field, ticker_taper());

    /// Wait for @OnRspUserLogout.
    m_parent->wait_logout();

    m_api->RegisterSpi(nullptr);
    m_api->Release();
}

int64_t trade::broker::CTPBrokerImpl::new_order(const std::shared_ptr<types::NewOrderReq>& new_order_req)
{
    const auto [request_seq, unique_id] = new_id_pair();

    CThostFtdcInputOrderField input_order_field {};

    /// SDK only accepts int32_t/std::string for OrderRef, so we need to map
    /// unique_id to int32_t.
    M_A {input_order_field.OrderRef} = std::to_string(request_seq);

    /// Fields in NewOrderReq.
    M_A {input_order_field.InstrumentID}  = new_order_req->symbol();
    M_A {input_order_field.ExchangeID}    = to_exchange(new_order_req->exchange());
    input_order_field.Direction           = to_side(new_order_req->side());
    input_order_field.CombOffsetFlag[0]   = to_position_side(new_order_req->position_side());
    input_order_field.LimitPrice          = new_order_req->price();
    input_order_field.VolumeTotalOriginal = static_cast<TThostFtdcVolumeType>(new_order_req->quantity());

    /// Fields required in docs.
    input_order_field.OrderPriceType      = THOST_FTDC_OPT_LimitPrice; /// 限价单
    input_order_field.TimeCondition       = THOST_FTDC_TC_GFD;         /// 当日有效

    M_A {input_order_field.BrokerID}      = m_broker_id;
    M_A {input_order_field.InvestorID}    = m_investor_id;

    input_order_field.ContingentCondition = THOST_FTDC_CC_Immediately;
    input_order_field.CombHedgeFlag[0]    = THOST_FTDC_HF_Speculation;

    input_order_field.VolumeCondition     = THOST_FTDC_VC_AV;
    input_order_field.TimeCondition       = THOST_FTDC_TC_GFD;
    input_order_field.MinVolume           = 1;
    input_order_field.ForceCloseReason    = THOST_FTDC_FCC_NotForceClose;

    /// @OnRspOrderInsert.
    const auto code = m_api->ReqOrderInsert(&input_order_field, request_seq);
    if (code != 0) {
        logger->error("Failed to call ReqOrderInsert: returned code {}", code);
        return INVALID_ID;
    }

    return unique_id;
}

void trade::broker::CTPBrokerImpl::init_req()
{
    auto [request_seq, request_id] = new_id_pair();

    /// @OnRspQryInvestor.
    CThostFtdcQryInvestorField qry_investor_field {};
    auto code = m_api->ReqQryInvestor(&qry_investor_field, request_seq);
    if (code != 0) {
        logger->error("Failed to call ReqQryInvestor: returned code {}", code);
    }
    logger->info("Queried investors in request {}", request_id);

    std::tie(request_seq, request_id) = new_id_pair();

    /// @OnRspQryExchange.
    CThostFtdcQryExchangeField qry_exchange_field {};
    code = m_api->ReqQryExchange(&qry_exchange_field, request_seq);
    if (code != 0) {
        logger->error("Failed to call ReqQryExchange: returned code {}", code);
    }
    logger->info("Queried exchanges in request {}", request_id);

    std::tie(request_seq, request_id) = new_id_pair();

    /// @OnReqQryProduct.
    CThostFtdcQryProductField qry_product_field {};
    code = m_api->ReqQryProduct(&qry_product_field, request_seq);
    if (code != 0) {
        logger->error("Failed to call ReqQryProduct: returned code {}", code);
    }
    logger->info("Queried products in request {}", request_id);

    std::tie(request_seq, request_id) = new_id_pair();

    /// @OnReqQryInstrument.
    CThostFtdcQryInstrumentField qry_instrument_field {};
    code = m_api->ReqQryInstrument(&qry_instrument_field, request_seq);
    if (code != 0) {
        logger->error("Failed to call ReqQryInstrument: returned code {}", code);
    }
    logger->info("Queried instruments in request {}", request_id);

    std::tie(request_seq, request_id) = new_id_pair();

    /// @OnReqQryTradingAccount.
    CThostFtdcQryTradingAccountField qry_trading_account_field {};
    code = m_api->ReqQryTradingAccount(&qry_trading_account_field, request_seq);
    if (code != 0) {
        logger->error("Failed to call ReqQryTradingAccount: returned code {}", code);
    }
    logger->info("Queried trading accounts in request {}", request_id);

    std::tie(request_seq, request_id) = new_id_pair();

    /// @OnQryInvestorPosition.
    CThostFtdcQryInvestorPositionField qry_investor_position_field {};
    code = m_api->ReqQryInvestorPosition(&qry_investor_position_field, request_seq);
    if (code != 0) {
        logger->error("Failed to call ReqQryInvestorPosition: returned code {}", code);
    }
    logger->info("Queried investor positions in request {}", request_id);

    std::tie(request_seq, request_id) = new_id_pair();

    /// @OnReqQryOrder.
    CThostFtdcQryOrderField qry_order_field {};
    code = m_api->ReqQryOrder(&qry_order_field, request_seq);
    if (code != 0) {
        logger->error("Failed to call ReqQryOrder: returned code {}", code);
    }
    logger->info("Queried orders in request {}", request_id);
}

#define pRspInfo_NULLPTR_CHECKER                                  \
    if (pRspInfo == nullptr) {                                    \
        logger->warn("A nullptr pRspInfo found in {}", __func__); \
        return;                                                   \
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

    pRspInfo_NULLPTR_CHECKER;

    if (pRspInfo->ErrorID != 0) {
        m_parent->notify_login_failure("Failed to login with code {}: {}", pRspInfo->ErrorID, utilities::GB2312ToUTF8()(pRspInfo->ErrorMsg));
        return;
    }

    M_A {m_broker_id} = pRspUserLogin->BrokerID;
    M_A {m_user_id}   = pRspUserLogin->UserID;

    logger->info("Logged to {} successfully as BrokerID/UserID {}/{} at {}-{}", pRspUserLogin->SystemName, pRspUserLogin->BrokerID, pRspUserLogin->UserID, pRspUserLogin->TradingDay, pRspUserLogin->LoginTime);

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

    pRspInfo_NULLPTR_CHECKER;

    if (pRspInfo->ErrorID != 0) {
        m_parent->notify_logout_failure("Failed to logout with code {}: {}", pRspInfo->ErrorID, pRspInfo->ErrorMsg);
        return;
    }

    logger->info("Logged out successfully as BrokerID/UserID {}/{}", pUserLogout->BrokerID, pUserLogout->UserID);

    m_parent->notify_logout_success();
}

/// @ReqQryInvestor.
void trade::broker::CTPBrokerImpl::OnRspQryInvestor(
    CThostFtdcInvestorField* pInvestor,
    CThostFtdcRspInfoField* pRspInfo,
    const int nRequestID, const bool bIsLast
)
{
    CThostFtdcTraderSpi::OnRspQryInvestor(pInvestor, pRspInfo, nRequestID, bIsLast);

    pRspInfo_NULLPTR_CHECKER;

    const auto request_id = get_by_seq_id(nRequestID);

    if (pRspInfo->ErrorID != 0) {
        logger->error("Failed to load investor for request {}: {}", request_id, pRspInfo->ErrorMsg);
        return;
    }

    M_A {m_investor_id} = pInvestor->InvestorID;

    logger->debug("Loaded investor {} - {}", pInvestor->InvestorID, utilities::GB2312ToUTF8()(pInvestor->InvestorName));

    if (bIsLast) {
        logger->info("Loaded investor id {} for request {}", m_investor_id, request_id);
    }
}

/// @ReqQryExchange.
void trade::broker::CTPBrokerImpl::OnRspQryExchange(
    CThostFtdcExchangeField* pExchange,
    CThostFtdcRspInfoField* pRspInfo,
    const int nRequestID, const bool bIsLast
)
{
    CThostFtdcTraderSpi::OnRspQryExchange(pExchange, pRspInfo, nRequestID, bIsLast);

    pRspInfo_NULLPTR_CHECKER;

    const auto request_id = get_by_seq_id(nRequestID);

    if (pRspInfo->ErrorID != 0) {
        logger->error("Failed to load exchange for request {}: {}", request_id, pRspInfo->ErrorMsg);
        return;
    }

    m_exchanges.emplace(pExchange->ExchangeID, *pExchange);

    logger->debug("Loaded exchange {} - {}", pExchange->ExchangeID, utilities::GB2312ToUTF8()(pExchange->ExchangeName));

    if (bIsLast) {
        logger->info("Loaded {} exchanges for request {}", m_exchanges.size(), request_id);
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

    pRspInfo_NULLPTR_CHECKER;

    const auto request_id = get_by_seq_id(nRequestID);

    if (pRspInfo->ErrorID != 0) {
        logger->error("Failed to load product for request {}: {}", request_id, pRspInfo->ErrorMsg);
        return;
    }

    m_products.emplace(pProduct->ProductID, *pProduct);

    logger->debug("Loaded product {} - {}", pProduct->ProductID, utilities::GB2312ToUTF8()(pProduct->ProductName));

    if (bIsLast) {
        logger->info("Loaded {} products for request {}", m_products.size(), request_id);
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

    pRspInfo_NULLPTR_CHECKER;

    const auto request_id = get_by_seq_id(nRequestID);

    if (pRspInfo->ErrorID != 0) {
        logger->error("Failed to load instrument for request {}: {}", request_id, pRspInfo->ErrorMsg);
        return;
    }

    m_instruments.emplace(pInstrument->InstrumentID, *pInstrument);

    logger->debug("Loaded instrument {} - {}", pInstrument->InstrumentID, utilities::GB2312ToUTF8()(pInstrument->InstrumentName));

    if (bIsLast) {
        logger->info("Loaded {} instruments for request {}", m_instruments.size(), request_id);
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

    pRspInfo_NULLPTR_CHECKER;

    const auto request_id = get_by_seq_id(nRequestID);

    if (pRspInfo->ErrorID != 0) {
        logger->error("Failed to load trading account for request {}: {}", request_id, pRspInfo->ErrorMsg);
        return;
    }

    /// pTradingAccount will be nullptr if there is no trading account.
    if (pTradingAccount == nullptr) {
        assert(bIsLast);
        logger->warn("No trading account loaded for request {}", request_id);
        return;
    }

    m_trading_account.emplace(pTradingAccount->BrokerID, *pTradingAccount);

    logger->debug("Loaded trading account {} with available funds {} {}", pTradingAccount->AccountID, pTradingAccount->Available, pTradingAccount->CurrencyID);

    /// For caching between multiple calls.
    static std::unordered_map<decltype(snow_flaker()), std::shared_ptr<types::Funds>> fund_cache;

    const auto funds = fund_cache.emplace(request_id, std::make_shared<types::Funds>()).first->second;

    types::Fund fund;

    fund.set_account_id(pTradingAccount->AccountID);
    fund.set_available_fund(pTradingAccount->Available);
    fund.set_withdrawn_fund(pTradingAccount->Withdraw);
    fund.set_frozen_fund(pTradingAccount->FrozenCash);
    fund.set_frozen_margin(pTradingAccount->FrozenMargin);
    fund.set_frozen_commission(pTradingAccount->FrozenCommission);

    funds->add_funds()->CopyFrom(fund);

    if (bIsLast) {
        logger->info("Loaded {} trading accounts for request {}", m_trading_account.size(), request_id);
        m_holder->init_funds(funds);
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

    pRspInfo_NULLPTR_CHECKER;

    const auto request_id = get_by_seq_id(nRequestID);

    if (pRspInfo->ErrorID != 0) {
        logger->error("Failed to load position for request {}: {}", request_id, pRspInfo->ErrorMsg);
        return;
    }

    /// pInvestorPosition will be nullptr if there is no position.
    if (pInvestorPosition == nullptr) {
        assert(bIsLast);
        logger->warn("No position loaded for request {}", request_id);
        return;
    }

    m_positions.emplace(pInvestorPosition->InstrumentID, *pInvestorPosition);

    logger->debug("Loaded position {} - {}", pInvestorPosition->InstrumentID, pInvestorPosition->Position);

    /// For caching between multiple calls.
    static std::unordered_map<decltype(snow_flaker()), std::shared_ptr<types::Positions>> position_cache;

    const auto positions = position_cache.emplace(request_id, std::make_shared<types::Positions>()).first->second;

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
    position.set_allocated_update_time(now());

    positions->add_positions()->CopyFrom(position);

    if (bIsLast) {
        logger->info("Loaded {} positions for request {}", m_positions.size(), request_id);
        m_holder->init_positions(positions);
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

    pRspInfo_NULLPTR_CHECKER;

    const auto request_id = get_by_seq_id(nRequestID);

    if (pRspInfo->ErrorID != 0) {
        logger->error("Failed to load order for request {}: {}", request_id, pRspInfo->ErrorMsg);
        return;
    }

    /// pOrder will be nullptr if there is no order.
    if (pOrder == nullptr) {
        assert(bIsLast);
        logger->warn("No order loaded for request {}", request_id);
        return;
    }

    m_orders.emplace(pOrder->OrderRef, *pOrder);

    logger->debug("Loaded order {} - {}", pOrder->OrderRef, utilities::GB2312ToUTF8()(pOrder->InstrumentID));

    if (bIsLast) {
        logger->info("Loaded {} orders for request {}", m_orders.size(), request_id);
    }
}

void trade::broker::CTPBrokerImpl::OnRspOrderInsert(
    CThostFtdcInputOrderField* pInputOrder,
    CThostFtdcRspInfoField* pRspInfo,
    const int nRequestID, const bool bIsLast
)
{
    CThostFtdcTraderSpi::OnRspOrderInsert(pInputOrder, pRspInfo, nRequestID, bIsLast);

    pRspInfo_NULLPTR_CHECKER;

    const auto request_id = get_by_seq_id(nRequestID);

    if (pRspInfo->ErrorID != 0) {
        logger->error("Failed to insert order for request {}: {}", request_id, utilities::GB2312ToUTF8()(pRspInfo->ErrorMsg));
        return;
    }

    logger->debug("Inserted order {} - {}", pInputOrder->OrderRef, utilities::GB2312ToUTF8()(pInputOrder->InstrumentID));
}

std::string trade::broker::CTPBrokerImpl::to_exchange(const types::ExchangeType exchange)
{
    switch (exchange) {
    case types::ExchangeType::bse: return "BSE";
    case types::ExchangeType::cffex: return "CFFEX";
    case types::ExchangeType::cme: return "CME";
    case types::ExchangeType::czce: return "CZCE";
    case types::ExchangeType::dce: return "DCE";
    case types::ExchangeType::gfex: return "GFEX";
    case types::ExchangeType::hkex: return "HKEX";
    case types::ExchangeType::ine: return "INE";
    case types::ExchangeType::nasd: return "NASD";
    case types::ExchangeType::nyse: return "NYSE";
    case types::ExchangeType::shfe: return "SHFE";
    case types::ExchangeType::sse: return "SSE";
    case types::ExchangeType::szse: return "SZSE";
    default: return "";
    }
}

TThostFtdcDirectionType trade::broker::CTPBrokerImpl::to_side(const types::SideType side)
{
    switch (side) {
    case types::SideType::buy: return THOST_FTDC_DEN_Buy;
    case types::SideType::sell: return THOST_FTDC_DEN_Sell;
    default: return '\0';
    }
}

char trade::broker::CTPBrokerImpl::to_position_side(const types::PositionSideType position_side)
{
    switch (position_side) {
    case types::PositionSideType::open: return THOST_FTDC_OFEN_Open;
    case types::PositionSideType::close: return THOST_FTDC_OFEN_Close;
    default: return '\0';
    }
}

/// Returns the current time with nanosecond precision.
///
/// Note: Set the returned value to a protobuf message will hand over the
/// ownership to the message, which takes the responsibility of freeing it.
google::protobuf::Timestamp* trade::broker::CTPBrokerImpl::now()
{
    return new google::protobuf::Timestamp(google::protobuf::util::TimeUtil::GetCurrentTime());
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
int64_t trade::broker::CTPBroker::new_order(const std::shared_ptr<types::NewOrderReq> new_order_req)
{
    BrokerProxy::new_order(new_order_req);
    return m_impl->new_order(new_order_req);
}
