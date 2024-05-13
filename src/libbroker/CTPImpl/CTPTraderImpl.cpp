#include <google/protobuf/util/time_util.h>
#include <regex>
#include <utility>

#include "libbroker/CTPImpl/CTPTraderImpl.h"
#include "utilities/GB2312ToUTF8.hpp"
#include "utilities/MakeAssignable.hpp"
#include "utilities/TimeHelper.hpp"

trade::broker::CTPTraderImpl::CTPTraderImpl(
    std::shared_ptr<ConfigType> config,
    std::shared_ptr<holder::IHolder> holder,
    std::shared_ptr<reporter::IReporter> reporter,
    utilities::LoginSyncer* parent
)
    : AppBase("CTPTraderImpl", std::move(config)),
      m_trader_api(CThostFtdcTraderApi::CreateFtdcTraderApi()),
      m_holder(std::move(holder)),
      m_reporter(std::move(reporter)),
      m_parent(parent)
{
    m_trader_api->RegisterSpi(this);
    m_trader_api->RegisterFront(const_cast<char*>(AppBase::config->get<std::string>("Server.TradeAddress").c_str()));

    m_trader_api->SubscribePublicTopic(THOST_TERT_RESTART);
    m_trader_api->SubscribePrivateTopic(THOST_TERT_RESTART);

    m_trader_api->Init();
}

trade::broker::CTPTraderImpl::~CTPTraderImpl()
{
    CThostFtdcUserLogoutField user_logout_field {};

    M_A {user_logout_field.BrokerID} = m_common_data.m_broker_id;
    M_A {user_logout_field.UserID}   = m_common_data.m_user_id;

    m_trader_api->ReqUserLogout(&user_logout_field, ticker_taper());

    /// Wait for @OnRspUserLogout.
    m_parent->wait_logout();

    m_trader_api->RegisterSpi(nullptr);
    m_trader_api->Release();
}

int64_t trade::broker::CTPTraderImpl::new_order(const std::shared_ptr<types::NewOrderReq>& new_order_req)
{
    const auto [request_seq, unique_id] = new_id_pair();

    CThostFtdcInputOrderField input_order_field {};

    /// SDK only accepts int32_t/std::string for OrderRef, so we need to map
    /// unique_id to int32_t.
    M_A {input_order_field.OrderRef} = std::to_string(request_seq);

    /// Fields in NewOrderReq.
    M_A {input_order_field.InstrumentID}  = new_order_req->symbol();
    M_A {input_order_field.ExchangeID}    = CTPCommonData::to_exchange(new_order_req->exchange());
    input_order_field.Direction           = CTPCommonData::to_side(new_order_req->side());
    input_order_field.CombOffsetFlag[0]   = CTPCommonData::to_position_side(new_order_req->position_side());
    input_order_field.LimitPrice          = new_order_req->price();
    input_order_field.VolumeTotalOriginal = static_cast<TThostFtdcVolumeType>(new_order_req->quantity());

    /// Fields required in docs.
    input_order_field.OrderPriceType      = THOST_FTDC_OPT_LimitPrice; /// 限价单
    input_order_field.TimeCondition       = THOST_FTDC_TC_GFD;         /// 当日有效

    M_A {input_order_field.BrokerID}      = m_common_data.m_broker_id;
    M_A {input_order_field.InvestorID}    = m_common_data.m_investor_id;

    input_order_field.ContingentCondition = THOST_FTDC_CC_Immediately;
    input_order_field.CombHedgeFlag[0]    = THOST_FTDC_HF_Speculation;

    input_order_field.VolumeCondition     = THOST_FTDC_VC_AV;
    input_order_field.TimeCondition       = THOST_FTDC_TC_GFD;
    input_order_field.MinVolume           = 1;
    input_order_field.ForceCloseReason    = THOST_FTDC_FCC_NotForceClose;

    /// @OnRspOrderInsert.
    const auto code = m_trader_api->ReqOrderInsert(&input_order_field, request_seq);
    if (code != 0) {
        logger->error("Failed to call ReqOrderInsert: returned code {}", code);
        return INVALID_ID;
    }

    return unique_id;
}

int64_t trade::broker::CTPTraderImpl::cancel_order(const std::shared_ptr<types::NewCancelReq>& new_cancel_req)
{
    const auto [request_seq, request_id] = new_id_pair();

    /// If original raw order id is not specified in request, query it from
    /// holder.
    if (!new_cancel_req->has_original_raw_order_id()) {
        const auto orders = m_holder->query_orders_by_unique_id(new_cancel_req->original_unique_id());

        if (orders->orders_size() != 1) {
            const auto cancel_order_rejection = std::make_shared<types::CancelOrderRejection>();

            cancel_order_rejection->set_original_unique_id(new_cancel_req->original_unique_id());
            cancel_order_rejection->clear_original_broker_id();
            cancel_order_rejection->clear_original_exchange_id();
            cancel_order_rejection->set_rejection_code(types::RejectionCode::unknown);
            cancel_order_rejection->set_rejection_reason(fmt::format("Order {} not found", new_cancel_req->original_unique_id()));
            cancel_order_rejection->set_allocated_rejection_time(utilities::Now<google::protobuf::Timestamp*>()());

            m_reporter->cancel_order_rejected(cancel_order_rejection);

            return INVALID_ID;
        }

        new_cancel_req->set_original_raw_order_id(orders->orders().at(0).exchange_id());
    }

    const auto [exchange_id, order_sys_id] = CTPCommonData::from_exchange_id(new_cancel_req->original_raw_order_id());

    CThostFtdcInputOrderActionField input_order_action_field {};

    /// SDK only accepts int32_t/std::string for OrderRef, so we need to map
    /// unique_id to int32_t.
    M_A {input_order_action_field.OrderRef} = std::to_string(request_seq);

    /// Fields in NewOrderReq.
    M_A {input_order_action_field.OrderSysID} = order_sys_id;
    M_A {input_order_action_field.ExchangeID} = exchange_id;

    /// Fields required in docs.
    input_order_action_field.ActionFlag       = THOST_FTDC_AF_Delete;

    M_A {input_order_action_field.BrokerID}   = m_common_data.m_broker_id;
    M_A {input_order_action_field.InvestorID} = m_common_data.m_investor_id;

    /// @OnRspOrderAction.
    const auto code = m_trader_api->ReqOrderAction(&input_order_action_field, request_seq);
    if (code != 0) {
        logger->error("Failed to call ReqOrderAction: returned code {}", code);
        return INVALID_ID;
    }

    return request_id;
}

void trade::broker::CTPTraderImpl::init_req()
{
    auto [request_seq, request_id] = new_id_pair();

    /// @OnRspQryInvestor.
    CThostFtdcQryInvestorField qry_investor_field {};
    auto code = m_trader_api->ReqQryInvestor(&qry_investor_field, request_seq);
    if (code != 0) {
        logger->error("Failed to call ReqQryInvestor: returned code {}", code);
    }
    logger->info("Queried investors in request {}", request_id);

    std::tie(request_seq, request_id) = new_id_pair();

    /// @OnRspQryExchange.
    CThostFtdcQryExchangeField qry_exchange_field {};
    code = m_trader_api->ReqQryExchange(&qry_exchange_field, request_seq);
    if (code != 0) {
        logger->error("Failed to call ReqQryExchange: returned code {}", code);
    }
    logger->info("Queried exchanges in request {}", request_id);

    std::tie(request_seq, request_id) = new_id_pair();

    /// @OnRspQryProduct.
    CThostFtdcQryProductField qry_product_field {};
    code = m_trader_api->ReqQryProduct(&qry_product_field, request_seq);
    if (code != 0) {
        logger->error("Failed to call ReqQryProduct: returned code {}", code);
    }
    logger->info("Queried products in request {}", request_id);

    std::tie(request_seq, request_id) = new_id_pair();

    /// @OnRspQryInstrument.
    CThostFtdcQryInstrumentField qry_instrument_field {};
    code = m_trader_api->ReqQryInstrument(&qry_instrument_field, request_seq);
    if (code != 0) {
        logger->error("Failed to call ReqQryInstrument: returned code {}", code);
    }
    logger->info("Queried instruments in request {}", request_id);

    std::tie(request_seq, request_id) = new_id_pair();

    /// @OnRspQryTradingAccount.
    CThostFtdcQryTradingAccountField qry_trading_account_field {};
    code = m_trader_api->ReqQryTradingAccount(&qry_trading_account_field, request_seq);
    if (code != 0) {
        logger->error("Failed to call ReqQryTradingAccount: returned code {}", code);
    }
    logger->info("Queried trading accounts in request {}", request_id);

    std::tie(request_seq, request_id) = new_id_pair();

    /// @OnRspQryInvestorPosition.
    CThostFtdcQryInvestorPositionField qry_investor_position_field {};
    code = m_trader_api->ReqQryInvestorPosition(&qry_investor_position_field, request_seq);
    if (code != 0) {
        logger->error("Failed to call ReqQryInvestorPosition: returned code {}", code);
    }
    logger->info("Queried investor positions in request {}", request_id);

    std::tie(request_seq, request_id) = new_id_pair();
}

#define NULLPTR_CHECKER(ptr)                                      \
    if (ptr == nullptr) {                                         \
        logger->warn("A nullptr {} found in {}", #ptr, __func__); \
        return;                                                   \
    }

void trade::broker::CTPTraderImpl::OnFrontConnected()
{
    CThostFtdcTraderSpi::OnFrontConnected();

    logger->info("Connected to CTP Trader server {} with version {}", config->get<std::string>("Server.TradeAddress"), CThostFtdcTraderApi::GetApiVersion());

    CThostFtdcReqUserLoginField req_user_login_field {};

    M_A {req_user_login_field.BrokerID} = config->get<std::string>("User.BrokerID");
    M_A {req_user_login_field.UserID}   = config->get<std::string>("User.UserID");
    M_A {req_user_login_field.Password} = config->get<std::string>("User.Password");

    /// @OnRspUserLogin.
    const auto code = m_trader_api->ReqUserLogin(&req_user_login_field, ticker_taper());
    if (code != 0) {
        m_parent->notify_login_failure("Failed to call ReqUserLogin: returned code {}", code);
    }
}

void trade::broker::CTPTraderImpl::OnRspUserLogin(
    CThostFtdcRspUserLoginField* pRspUserLogin,
    CThostFtdcRspInfoField* pRspInfo,
    const int nRequestID, const bool bIsLast
)
{
    CThostFtdcTraderSpi::OnRspUserLogin(pRspUserLogin, pRspInfo, nRequestID, bIsLast);

    NULLPTR_CHECKER(pRspInfo);

    if (pRspInfo->ErrorID != 0) {
        m_parent->notify_login_failure("Failed to login with code {}: {}", pRspInfo->ErrorID, utilities::GB2312ToUTF8()(pRspInfo->ErrorMsg));
        return;
    }

    M_A {m_common_data.m_broker_id} = pRspUserLogin->BrokerID;
    M_A {m_common_data.m_user_id}   = pRspUserLogin->UserID;
    m_common_data.m_front_id        = pRspUserLogin->FrontID;
    m_common_data.m_session_id      = pRspUserLogin->SessionID;

    try {
        /// Set OrderRef starts from MaxOrderRef for avoiding duplicated
        /// OrderRef in same session.
        /// TThostFtdcOrderRefType is char[13], so we can use std::stoi.
        /// UINT32_MAX is 4,294,967,295.
        ticker_taper.reset(std::stoi(pRspUserLogin->MaxOrderRef));
    }
    catch (const std::exception& e) {
        m_parent->notify_login_failure("Can not parse MaxOrderRef {}: {}", pRspUserLogin->MaxOrderRef, e.what());
    }

    logger->info("Logged to {} successfully as BrokerID/UserID {}/{} with FrontID/SessionID {}/{} at {}-{}", pRspUserLogin->SystemName, pRspUserLogin->BrokerID, pRspUserLogin->UserID, pRspUserLogin->FrontID, pRspUserLogin->SessionID, pRspUserLogin->TradingDay, pRspUserLogin->LoginTime);

    m_parent->notify_login_success();

    /// Initial queries.
    init_req();
}

void trade::broker::CTPTraderImpl::OnRspUserLogout(
    CThostFtdcUserLogoutField* pUserLogout,
    CThostFtdcRspInfoField* pRspInfo,
    const int nRequestID, const bool bIsLast
)
{
    CThostFtdcTraderSpi::OnRspUserLogout(pUserLogout, pRspInfo, nRequestID, bIsLast);

    NULLPTR_CHECKER(pRspInfo);

    if (pRspInfo->ErrorID != 0) {
        m_parent->notify_logout_failure("Failed to logout with code {}: {}", pRspInfo->ErrorID, pRspInfo->ErrorMsg);
        return;
    }

    logger->info("Logged out successfully as BrokerID/UserID {}/{} with FrontID/SessionID {}/{}", pUserLogout->BrokerID, pUserLogout->UserID, m_common_data.m_front_id, m_common_data.m_session_id);

    m_parent->notify_logout_success();
}

/// @ReqQryInvestor.
void trade::broker::CTPTraderImpl::OnRspQryInvestor(
    CThostFtdcInvestorField* pInvestor,
    CThostFtdcRspInfoField* pRspInfo,
    const int nRequestID, const bool bIsLast
)
{
    CThostFtdcTraderSpi::OnRspQryInvestor(pInvestor, pRspInfo, nRequestID, bIsLast);

    NULLPTR_CHECKER(pRspInfo);

    const auto request_id = get_by_seq_id(nRequestID);

    if (pRspInfo->ErrorID != 0) {
        logger->error("Failed to load investor for request {}: {}", request_id.value_or(INVALID_ID), utilities::GB2312ToUTF8()(pRspInfo->ErrorMsg));
        return;
    }

    M_A {m_common_data.m_investor_id} = pInvestor->InvestorID;

    if (bIsLast) {
        logger->info("Loaded investor id {} for request {}", m_common_data.m_investor_id, request_id.value_or(INVALID_ID));
    }
}

/// @ReqQryExchange.
void trade::broker::CTPTraderImpl::OnRspQryExchange(
    CThostFtdcExchangeField* pExchange,
    CThostFtdcRspInfoField* pRspInfo,
    const int nRequestID, const bool bIsLast
)
{
    CThostFtdcTraderSpi::OnRspQryExchange(pExchange, pRspInfo, nRequestID, bIsLast);

    NULLPTR_CHECKER(pRspInfo);

    const auto request_id = get_by_seq_id(nRequestID);

    if (pRspInfo->ErrorID != 0) {
        logger->error("Failed to load exchange for request {}: {}", request_id.value_or(INVALID_ID), pRspInfo->ErrorMsg);
        return;
    }

    logger->debug("Loaded exchange {}({}) with property {}", utilities::GB2312ToUTF8()(pExchange->ExchangeName), pExchange->ExchangeID, pExchange->ExchangeProperty);

    m_exchanges.emplace(pExchange->ExchangeID, *pExchange);

    if (bIsLast) {
        logger->info("Loaded {} exchanges for request {}", m_exchanges.size(), request_id.value_or(INVALID_ID));
    }
}

/// @ReqQryProduct.
void trade::broker::CTPTraderImpl::OnRspQryProduct(
    CThostFtdcProductField* pProduct,
    CThostFtdcRspInfoField* pRspInfo,
    const int nRequestID, const bool bIsLast
)
{
    CThostFtdcTraderSpi::OnRspQryProduct(pProduct, pRspInfo, nRequestID, bIsLast);

    NULLPTR_CHECKER(pRspInfo);

    const auto request_id = get_by_seq_id(nRequestID);

    if (pRspInfo->ErrorID != 0) {
        logger->error("Failed to load product for request {}: {}", request_id.value_or(INVALID_ID), pRspInfo->ErrorMsg);
        return;
    }

    logger->debug("Loaded product {}({})", utilities::GB2312ToUTF8()(pProduct->ProductName), pProduct->ProductID);

    m_products.emplace(pProduct->ProductID, *pProduct);

    if (bIsLast) {
        logger->info("Loaded {} products for request {}", m_products.size(), request_id.value_or(INVALID_ID));
    }
}

/// @ReqQryInstrument.
void trade::broker::CTPTraderImpl::OnRspQryInstrument(
    CThostFtdcInstrumentField* pInstrument,
    CThostFtdcRspInfoField* pRspInfo,
    const int nRequestID, const bool bIsLast
)
{
    CThostFtdcTraderSpi::OnRspQryInstrument(pInstrument, pRspInfo, nRequestID, bIsLast);

    NULLPTR_CHECKER(pRspInfo);

    const auto request_id = get_by_seq_id(nRequestID);

    if (pRspInfo->ErrorID != 0) {
        logger->error("Failed to load instrument for request {}: {}", request_id.value_or(INVALID_ID), pRspInfo->ErrorMsg);
        return;
    }

    logger->debug("Loaded instrument {}({})", utilities::GB2312ToUTF8()(pInstrument->InstrumentName), pInstrument->InstrumentID);

    /// For caching between multiple calls.
    static std::unordered_map<decltype(snow_flaker()), std::shared_ptr<types::Symbols>> symbol_cache;

    const auto symbols = symbol_cache.emplace(request_id.value_or(INVALID_ID), std::make_shared<types::Symbols>()).first->second;

    types::Symbol symbol;

    symbol.set_symbol(pInstrument->InstrumentID);
    symbol.set_exchange(CTPCommonData::to_exchange(pInstrument->ExchangeID));

    symbols->add_symbols()->CopyFrom(symbol);

    if (bIsLast) {
        logger->info("Loaded {} instruments for request {}", symbols->symbols_size(), request_id.value_or(INVALID_ID));
        m_holder->update_symbols(symbols);
        symbol_cache.erase(request_id.value_or(INVALID_ID));
    }
}

/// @ReqQryTradingAccount.
void trade::broker::CTPTraderImpl::OnRspQryTradingAccount(
    CThostFtdcTradingAccountField* pTradingAccount,
    CThostFtdcRspInfoField* pRspInfo,
    const int nRequestID, const bool bIsLast
)
{
    CThostFtdcTraderSpi::OnRspQryTradingAccount(pTradingAccount, pRspInfo, nRequestID, bIsLast);

    NULLPTR_CHECKER(pRspInfo);

    const auto request_id = get_by_seq_id(nRequestID);

    if (pRspInfo->ErrorID != 0) {
        logger->error("Failed to load trading account for request {}: {}", request_id.value_or(INVALID_ID), pRspInfo->ErrorMsg);
        return;
    }

    /// pTradingAccount will be nullptr if there is no trading account.
    if (pTradingAccount == nullptr) {
        assert(bIsLast);
        logger->warn("No trading account loaded for request {}", request_id.value_or(INVALID_ID));
        return;
    }

    logger->debug("Loaded trading account {} with available funds {} {}", pTradingAccount->AccountID, pTradingAccount->Available, pTradingAccount->CurrencyID);

    m_trading_account.emplace(pTradingAccount->BrokerID, *pTradingAccount);

    /// For caching between multiple calls.
    static std::unordered_map<decltype(snow_flaker()), std::shared_ptr<types::Funds>> fund_cache;

    const auto funds = fund_cache.emplace(request_id.value_or(INVALID_ID), std::make_shared<types::Funds>()).first->second;

    types::Fund fund;

    fund.set_account_id(pTradingAccount->AccountID);
    fund.set_available_fund(pTradingAccount->Available);
    fund.set_withdrawn_fund(pTradingAccount->Withdraw);
    fund.set_frozen_fund(pTradingAccount->FrozenCash);
    fund.set_frozen_margin(pTradingAccount->FrozenMargin);
    fund.set_frozen_commission(pTradingAccount->FrozenCommission);

    funds->add_funds()->CopyFrom(fund);

    if (bIsLast) {
        logger->info("Loaded {} trading accounts for request {}", m_trading_account.size(), request_id.value_or(INVALID_ID));
        m_holder->update_funds(funds);
        fund_cache.erase(request_id.value_or(INVALID_ID));
    }
}

/// @ReqQryInvestorPosition.
void trade::broker::CTPTraderImpl::OnRspQryInvestorPosition(
    CThostFtdcInvestorPositionField* pInvestorPosition,
    CThostFtdcRspInfoField* pRspInfo,
    const int nRequestID, const bool bIsLast
)
{
    CThostFtdcTraderSpi::OnRspQryInvestorPosition(pInvestorPosition, pRspInfo, nRequestID, bIsLast);

    NULLPTR_CHECKER(pRspInfo);

    const auto request_id = get_by_seq_id(nRequestID);

    if (pRspInfo->ErrorID != 0) {
        logger->error("Failed to load position for request {}: {}", request_id.value_or(INVALID_ID), pRspInfo->ErrorMsg);
        return;
    }

    /// pInvestorPosition will be nullptr if there is no position.
    if (pInvestorPosition == nullptr) {
        assert(bIsLast);
        logger->warn("No position loaded for request {}", request_id.value_or(INVALID_ID));
        return;
    }

    logger->debug("Loaded position {} - {}", pInvestorPosition->InstrumentID, pInvestorPosition->Position);

    /// For caching between multiple calls.
    static std::unordered_map<decltype(snow_flaker()), std::shared_ptr<types::Positions>> position_cache;

    const auto positions = position_cache.emplace(request_id.value_or(INVALID_ID), std::make_shared<types::Positions>()).first->second;

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
    position.set_allocated_update_time(utilities::Now<google::protobuf::Timestamp*>()());

    positions->add_positions()->CopyFrom(position);

    if (bIsLast) {
        logger->info("Loaded {} positions for request {}", positions->positions_size(), request_id.value_or(INVALID_ID));
        m_holder->update_positions(positions);
        position_cache.erase(request_id.value_or(INVALID_ID));
    }
}

/// @ReqOrderInsert.
/// Local order contains only, rejected by front end.
void trade::broker::CTPTraderImpl::OnRspOrderInsert(
    CThostFtdcInputOrderField* pInputOrder,
    CThostFtdcRspInfoField* pRspInfo,
    const int nRequestID, const bool bIsLast
)
{
    CThostFtdcTraderSpi::OnRspOrderInsert(pInputOrder, pRspInfo, nRequestID, bIsLast);

    NULLPTR_CHECKER(pRspInfo);

    const auto unique_id = get_by_seq_id(nRequestID);

    /// This callback is called only when an order is rejected by front end.
    /// Original description:
    /// 综合交易平台交易核心返回的包含错误信息的报单响应 …… 交易前置从交易核心订
    /// 阅到错误的报单响应报文，以对话模式将该报文转发给交易终端。
    assert(pRspInfo->ErrorID != 0);

    const auto order_rejection = std::make_shared<types::OrderRejection>();

    order_rejection->set_original_unique_id(unique_id.value_or(INVALID_ID));
    if (pInputOrder != nullptr) {
        order_rejection->set_original_broker_id(CTPCommonData::to_broker_id(m_common_data.m_front_id, m_common_data.m_session_id, pInputOrder->OrderRef));
        order_rejection->set_original_exchange_id(CTPCommonData::to_exchange_id(pInputOrder->ExchangeID, "")); /// No OrderSysRef here.
    }
    order_rejection->set_rejection_code(types::RejectionCode::unknown);
    order_rejection->set_rejection_reason(utilities::GB2312ToUTF8()(pRspInfo->ErrorMsg));
    order_rejection->set_allocated_rejection_time(utilities::Now<google::protobuf::Timestamp*>()());

    m_reporter->order_rejected(order_rejection);
}

/// Outside order (order submitted from other session) contains, rejected by
/// exchange.
void trade::broker::CTPTraderImpl::OnErrRtnOrderInsert(
    CThostFtdcInputOrderField* pInputOrder,
    CThostFtdcRspInfoField* pRspInfo
)
{
    CThostFtdcTraderSpi::OnErrRtnOrderInsert(pInputOrder, pRspInfo);

    NULLPTR_CHECKER(pRspInfo);

    /// This callback is called only when an order is rejected by exchange.
    /// See https://zhuanlan.zhihu.com/p/94507929 for more details.
    assert(pRspInfo->ErrorID != 0);

    logger->warn("An outside order {} from ip {} mac {} is rejected by exchange: {}", pInputOrder->OrderRef, pInputOrder->IPAddress, pInputOrder->MacAddress, pRspInfo->ErrorMsg);
}

void trade::broker::CTPTraderImpl::OnRspOrderAction(
    CThostFtdcInputOrderActionField* pInputOrderAction,
    CThostFtdcRspInfoField* pRspInfo,
    const int nRequestID, const bool bIsLast
)
{
    CThostFtdcTraderSpi::OnRspOrderAction(pInputOrderAction, pRspInfo, nRequestID, bIsLast);

    NULLPTR_CHECKER(pRspInfo);

    const auto request_id = get_by_seq_id(nRequestID);

    assert(pRspInfo->ErrorID != 0);

    const auto cancel_order_rejection = std::make_shared<types::CancelOrderRejection>();

    cancel_order_rejection->set_original_unique_id(request_id.value_or(INVALID_ID));
    if (pInputOrderAction != nullptr) {
        cancel_order_rejection->set_original_broker_id(CTPCommonData::to_broker_id(pInputOrderAction->FrontID, pInputOrderAction->SessionID, pInputOrderAction->OrderRef));
        cancel_order_rejection->set_original_exchange_id(CTPCommonData::to_exchange_id(pInputOrderAction->ExchangeID, pInputOrderAction->OrderSysID));
    }
    cancel_order_rejection->set_rejection_code(types::RejectionCode::unknown);
    cancel_order_rejection->set_rejection_reason(utilities::GB2312ToUTF8()(pRspInfo->ErrorMsg));
    cancel_order_rejection->set_allocated_rejection_time(utilities::Now<google::protobuf::Timestamp*>()());

    m_reporter->cancel_order_rejected(cancel_order_rejection);
}

void trade::broker::CTPTraderImpl::OnErrRtnOrderAction(CThostFtdcOrderActionField* pOrderAction, CThostFtdcRspInfoField* pRspInfo)
{
    CThostFtdcTraderSpi::OnErrRtnOrderAction(pOrderAction, pRspInfo);

    NULLPTR_CHECKER(pRspInfo);

    assert(pRspInfo->ErrorID != 0);

    logger->warn("An outside cancel {} from ip {} mac {} is rejected by exchange: {}", CTPCommonData::to_exchange_id(pOrderAction->ExchangeID, pOrderAction->OrderSysID), pOrderAction->IPAddress, pOrderAction->MacAddress, pRspInfo->ErrorMsg);
}

void trade::broker::CTPTraderImpl::OnRtnOrder(CThostFtdcOrderField* pOrder)
{
    CThostFtdcTraderSpi::OnRtnOrder(pOrder);

    NULLPTR_CHECKER(pOrder);

    logger->debug("New CThostFtdcOrder {} arrived with status {} and submit status {}", CTPCommonData::to_exchange_id(pOrder->ExchangeID, pOrder->OrderSysID), pOrder->OrderStatus, pOrder->OrderSubmitStatus);

    using OrderRefType = std::tuple_element_t<0, decltype(new_id_pair())>;
    OrderRefType order_ref;

    try {
        /// TThostFtdcOrderRefType is char[13], so we can use std::stoi.
        /// UINT32_MAX is 4,294,967,295.
        order_ref = std::stoi(pOrder->OrderRef);
    }
    catch (const std::exception& e) {
        logger->warn("Can not convert OrderRef {} to OrderRefType: {}", pOrder->OrderRef, e.what());
        return;
    }

    auto unique_id = get_by_seq_id(order_ref);

    std::shared_ptr<types::Orders> orders;
    types::Order order;

    /// No unique_id in AppBase means that the order not submitted by this
    /// instance.
    if (!unique_id.has_value()) {
        /// Check if the order is already in holder.
        orders = m_holder->query_orders_by_exchange_id(CTPCommonData::to_exchange_id(pOrder->ExchangeID, pOrder->OrderSysID));
        if (orders->orders().empty()) {
            /// Insert the new outside order into holder.
            order.set_unique_id(snow_flaker());
            order.set_broker_id(CTPCommonData::to_broker_id(m_common_data.m_front_id, m_common_data.m_session_id, pOrder->OrderRef));
            order.set_exchange_id(CTPCommonData::to_exchange_id(pOrder->ExchangeID, pOrder->OrderSysID));
            order.set_symbol(pOrder->InstrumentID);
            order.set_side(CTPCommonData::to_side(pOrder->Direction));
            order.set_position_side(CTPCommonData::to_position_side(pOrder->CombOffsetFlag[0]));
            order.set_price(pOrder->LimitPrice);
            order.set_quantity(pOrder->VolumeTotalOriginal);
            order.set_allocated_creation_time(utilities::Now<google::protobuf::Timestamp*>()());

            orders->add_orders()->CopyFrom(order);

            m_holder->update_orders(orders);

            logger->info("Assigned new unique_id {} to new outside order {} from ip {} mac {} and inserted into holder", order.unique_id(), CTPCommonData::to_exchange_id(pOrder->ExchangeID, pOrder->OrderSysID), pOrder->IPAddress, pOrder->MacAddress);
        }
        else {
            /// Use queried order from holder.
            assert(orders->orders_size() == 1);
            order     = orders->mutable_orders()->at(0);
            unique_id = std::make_optional(order.unique_id());
        }
    }
    else {
        /// Use unique_id to query from holder.
        orders = m_holder->query_orders_by_unique_id(unique_id.value());

        /// No broker_id/exchange_id in holder means that the order is newly
        /// accepted.
        if (order.broker_id().empty()) {
            const auto broker_acceptance = std::make_shared<types::BrokerAcceptance>();

            broker_acceptance->set_unique_id(unique_id.value());
            broker_acceptance->set_broker_id(order.broker_id());
            broker_acceptance->set_allocated_broker_acceptance_time(utilities::Now<google::protobuf::Timestamp*>()());

            m_reporter->broker_accepted(broker_acceptance);

            const auto exchange_acceptance = std::make_shared<types::ExchangeAcceptance>();

            exchange_acceptance->set_unique_id(unique_id.value());
            exchange_acceptance->set_exchange_id(order.exchange_id());
            exchange_acceptance->set_allocated_exchange_acceptance_time(utilities::Now<google::protobuf::Timestamp*>()());

            m_reporter->exchange_accepted(exchange_acceptance);

            /// Update broker_id/exchange_id.
            order.set_broker_id(CTPCommonData::to_broker_id(pOrder->FrontID, pOrder->SessionID, pOrder->OrderRef));
            order.set_exchange_id(CTPCommonData::to_exchange_id(pOrder->ExchangeID, pOrder->OrderSysID));
            order.set_allocated_update_time(utilities::Now<google::protobuf::Timestamp*>()());

            m_holder->update_orders(orders);
        }
    }

    switch (pOrder->OrderStatus) {
    case THOST_FTDC_OST_Canceled: {
        switch (pOrder->OrderSubmitStatus) {
            /// If new order is rejected by exchange.
        case THOST_FTDC_OSS_InsertRejected: {
            const auto order_rejection = std::make_shared<types::OrderRejection>();

            order_rejection->set_original_unique_id(unique_id.value());
            order_rejection->set_original_broker_id(CTPCommonData::to_broker_id(pOrder->FrontID, pOrder->SessionID, pOrder->OrderRef));
            order_rejection->set_original_exchange_id(CTPCommonData::to_broker_id(pOrder->FrontID, pOrder->SessionID, pOrder->OrderRef));
            order_rejection->set_rejection_code(types::RejectionCode::unknown);
            order_rejection->set_rejection_reason(utilities::GB2312ToUTF8()(pOrder->StatusMsg));
            order_rejection->set_allocated_rejection_time(utilities::Now<google::protobuf::Timestamp*>()());

            m_reporter->order_rejected(order_rejection);

            break;
        }
            /// If cancel order is rejected by exchange.
        case THOST_FTDC_OSS_CancelRejected: {
            const auto cancel_order_rejection = std::make_shared<types::CancelOrderRejection>();

            cancel_order_rejection->set_original_unique_id(unique_id.value());
            cancel_order_rejection->set_original_broker_id(CTPCommonData::to_broker_id(pOrder->FrontID, pOrder->SessionID, pOrder->OrderRef));
            cancel_order_rejection->set_original_exchange_id(CTPCommonData::to_broker_id(pOrder->FrontID, pOrder->SessionID, pOrder->OrderRef));
            cancel_order_rejection->set_rejection_code(types::RejectionCode::unknown);
            cancel_order_rejection->set_rejection_reason(utilities::GB2312ToUTF8()(pOrder->StatusMsg));
            cancel_order_rejection->set_allocated_rejection_time(utilities::Now<google::protobuf::Timestamp*>()());

            m_reporter->cancel_order_rejected(cancel_order_rejection);

            break;
        }
        default: break;
        }
    default: break;
    }
    }
}
