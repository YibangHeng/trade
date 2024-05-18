#include <google/protobuf/util/time_util.h>
#include <regex>
#include <utility>

#include "libbroker/CUTImpl/CUTTraderImpl.h"
#include "utilities/GB2312ToUTF8.hpp"
#include "utilities/MakeAssignable.hpp"
#include "utilities/TimeHelper.hpp"

trade::broker::CUTTraderImpl::CUTTraderImpl(
    std::shared_ptr<ConfigType> config,
    std::shared_ptr<holder::IHolder> holder,
    std::shared_ptr<reporter::IReporter> reporter,
    utilities::LoginSyncer* parent
)
    : AppBase("CUTTraderImpl", std::move(config)),
      m_trader_api(CUTApi::CreateApi()),
      m_holder(std::move(holder)),
      m_reporter(std::move(reporter)),
      m_parent(parent)
{
    m_trader_api->RegisterSpi(this);
    m_trader_api->RegisterFront(const_cast<char*>(AppBase::config->get<std::string>("Server.TradeAddress").c_str()));

    m_trader_api->SubscribePublicTopic(UT_TERT_RESTART);
    m_trader_api->SubscribePrivateTopic(UT_TERT_RESTART);

    m_trader_api->Init();
}

trade::broker::CUTTraderImpl::~CUTTraderImpl()
{
    CUTReqLogoutField user_logout_field {};

    M_A {user_logout_field.UserID} = m_common_data.m_user_id;

    m_trader_api->ReqLogout(&user_logout_field, ticker_taper());

    /// Wait for @OnRspUserLogout.
    m_parent->wait_logout();

    m_trader_api->RegisterSpi(nullptr);
    m_trader_api->Release();
}

void trade::broker::CUTTraderImpl::new_order(
    const std::shared_ptr<types::NewOrderReq>& new_order_req,
    const std::shared_ptr<types::NewOrderRsp>& new_order_rsp
)
{
    const auto [request_seq, unique_id] = new_id_pair();

    CUTInputOrderField input_order_field {};

    input_order_field.OrderRef = request_seq;

    /// Fields in NewOrderReq.
    M_A {input_order_field.InstrumentID}  = new_order_req->symbol();
    input_order_field.ExchangeID          = CUTCommonData::to_exchange(new_order_req->exchange());
    input_order_field.Direction           = CUTCommonData::to_side(new_order_req->side());
    input_order_field.OffsetFlag          = CUTCommonData::to_position_side(new_order_req->position_side());
    input_order_field.LimitPrice          = new_order_req->price();
    input_order_field.VolumeTotalOriginal = static_cast<int>(new_order_req->quantity());

    /// Fields required in docs.
    input_order_field.OrderPriceType   = UT_OPT_LimitPrice; /// 限价单
    input_order_field.TimeCondition    = UT_TC_GFD;         /// 当日有效)

    M_A {input_order_field.InvestorID} = m_common_data.m_investor_id;

    input_order_field.HedgeFlag        = UT_HF_Speculation;

    input_order_field.VolumeCondition  = UT_VC_AV;
    input_order_field.TimeCondition    = UT_TC_GFD;

    /// @OnRspOrderInsert.
    const auto code = m_trader_api->ReqOrderInsert(&input_order_field, request_seq);
    if (code != 0) {
        logger->error("Failed to call ReqOrderInsert: returned code {}", code);

        new_order_rsp->set_request_id(new_order_req->request_id());
        new_order_rsp->set_unique_id(INVALID_ID);
        new_order_rsp->set_allocated_creation_time(utilities::Now<google::protobuf::Timestamp*>()());
        new_order_rsp->set_rejection_code(types::RejectionCode::unknown);
        new_order_rsp->set_rejection_reason("CUT API returned with code {}", code);

        return;
    }

    new_order_rsp->set_request_id(new_order_req->request_id());
    new_order_rsp->set_unique_id(new_order_req->unique_id());
    new_order_rsp->set_allocated_creation_time(utilities::Now<google::protobuf::Timestamp*>()());
}

void trade::broker::CUTTraderImpl::cancel_order(
    const std::shared_ptr<types::NewCancelReq>& new_cancel_req,
    const std::shared_ptr<types::NewCancelRsp>& new_cancel_rsp
)
{
    const auto [request_seq, request_id] = new_id_pair();

    /// If original raw order id is not specified in request, query it from
    /// holder.
    if (!new_cancel_req->has_original_raw_order_id()) {
        const auto orders = m_holder->query_orders_by_unique_id(new_cancel_req->original_unique_id());

        if (orders->orders_size() != 1) {
            new_cancel_rsp->set_request_id(new_cancel_req->request_id());
            new_cancel_rsp->set_original_unique_id(new_cancel_req->original_unique_id());
            new_cancel_rsp->set_allocated_creation_time(utilities::Now<google::protobuf::Timestamp*>()());
            new_cancel_rsp->set_rejection_code(types::RejectionCode::unknown);
            new_cancel_rsp->set_rejection_reason("Order {} not found", new_cancel_req->original_unique_id());

            return;
        }

        new_cancel_req->set_original_raw_order_id(orders->orders().at(0).broker_id());
    }

    const auto [exchange_id, order_sys_id] = CUTCommonData::from_exchange_id(new_cancel_req->original_raw_order_id());

    CUTInputOrderActionField input_order_action_field {};

    input_order_action_field.OrderRef = request_seq;

    /// Fields in NewCancelReq.
    const auto [front_id, session_id, order_ref] = CUTCommonData::from_broker_id(new_cancel_req->original_raw_order_id());

    try {
        input_order_action_field.OrderRef  = std::stoi(order_ref);
        input_order_action_field.FrontID   = std::stoi(front_id);
        input_order_action_field.SessionID = std::stoll(session_id);
    }
    catch (const std::exception& e) {
        logger->error("Failed to parse FrontID/SessionID/OrderRef from broker id {}: {}", new_cancel_req->original_raw_order_id(), e.what());

        new_cancel_rsp->set_request_id(new_cancel_req->request_id());
        new_cancel_rsp->set_original_unique_id(new_cancel_req->original_unique_id());
        new_cancel_rsp->set_allocated_creation_time(utilities::Now<google::protobuf::Timestamp*>()());
        new_cancel_rsp->set_rejection_code(types::RejectionCode::unknown);
        new_cancel_rsp->set_rejection_reason("Order {} not found", new_cancel_req->original_unique_id());

        return;
    }

    /// Fields required in docs.
    input_order_action_field.ActionFlag = UT_AF_Delete;

    /// @OnRspOrderAction.
    const auto code = m_trader_api->ReqOrderAction(&input_order_action_field, request_seq);
    if (code != 0) {
        logger->error("Failed to call ReqOrderAction: returned code {}", code);

        new_cancel_rsp->set_request_id(new_cancel_req->request_id());
        new_cancel_rsp->set_original_unique_id(new_cancel_req->original_unique_id());
        new_cancel_rsp->set_allocated_creation_time(utilities::Now<google::protobuf::Timestamp*>()());
        new_cancel_rsp->set_rejection_code(types::RejectionCode::unknown);
        new_cancel_rsp->set_rejection_reason("Failed to call ReqOrderAction: returned code {}", code);

        return;
    }

    new_cancel_rsp->set_request_id(new_cancel_req->request_id());
    new_cancel_rsp->set_original_unique_id(new_cancel_req->original_unique_id());
    new_cancel_rsp->set_allocated_creation_time(utilities::Now<google::protobuf::Timestamp*>()());
}

void trade::broker::CUTTraderImpl::init_req()
{
    auto [request_seq, request_id] = new_id_pair();

    /// @OnRspQryInvestor.
    CUTQryInvestorField qry_investor_field {};
    auto code = m_trader_api->ReqQryInvestor(&qry_investor_field, request_seq);
    if (code != 0) {
        logger->error("Failed to call ReqQryInvestor: returned code {}", code);
    }
    logger->info("Queried investors in request {}", request_id);

    std::tie(request_seq, request_id) = new_id_pair();

    std::tie(request_seq, request_id) = new_id_pair();

    /// @OnRspQryInstrument.
    CUTQryInstrumentField qry_instrument_field {};
    code = m_trader_api->ReqQryInstrument(&qry_instrument_field, request_seq);
    if (code != 0) {
        logger->error("Failed to call ReqQryInstrument: returned code {}", code);
    }
    logger->info("Queried instruments in request {}", request_id);

    std::tie(request_seq, request_id) = new_id_pair();
    //
    // /// @OnRspQryTradingAccount.
    // CUTQryTradingAccountField qry_trading_account_field {};
    // code = m_trader_api->ReqQryTradingAccount(&qry_trading_account_field, request_seq);
    // if (code != 0) {
    //     logger->error("Failed to call ReqQryTradingAccount: returned code {}", code);
    // }
    // logger->info("Queried trading accounts in request {}", request_id);
    //
    // std::tie(request_seq, request_id) = new_id_pair();
    //
    // /// @OnRspQryInvestorPosition.
    // CUTQryInvestorPositionField qry_investor_position_field {};
    // code = m_trader_api->ReqQryInvestorPosition(&qry_investor_position_field, request_seq);
    // if (code != 0) {
    //     logger->error("Failed to call ReqQryInvestorPosition: returned code {}", code);
    // }
    // logger->info("Queried investor positions in request {}", request_id);
    //
    // std::tie(request_seq, request_id) = new_id_pair();
}

#define NULLPTR_CHECKER(ptr)                                      \
    if (ptr == nullptr) {                                         \
        logger->warn("A nullptr {} found in {}", #ptr, __func__); \
        return;                                                   \
    }

void trade::broker::CUTTraderImpl::OnFrontConnected()
{
    CUTSpi::OnFrontConnected();

    logger->info("Connected to CUT Trader server {} with version {}", config->get<std::string>("Server.TradeAddress"), CUTApi::GetApiVersion());

    CUTReqLoginField req_user_login_field {};

    M_A {req_user_login_field.UserID}   = config->get<std::string>("User.UserID");
    M_A {req_user_login_field.Password} = config->get<std::string>("User.Password");

    /// @OnRspUserLogin.
    const auto code = m_trader_api->ReqLogin(&req_user_login_field, ticker_taper());
    if (code != 0) {
        m_parent->notify_login_failure("Failed to call ReqUserLogin: returned code {}", code);
    }
}

void trade::broker::CUTTraderImpl::OnRspLogin(
    CUTRspLoginField* pRspLogin,
    CUTRspInfoField* pRspInfo,
    int nRequestID, bool bIsLast
)
{
    CUTSpi::OnRspLogin(pRspLogin, pRspInfo, nRequestID, bIsLast);

    NULLPTR_CHECKER(pRspInfo);

    if (pRspInfo->ErrorID != 0) {
        m_parent->notify_login_failure("Failed to login with code {}: {}", pRspInfo->ErrorID, utilities::GB2312ToUTF8()(pRspInfo->ErrorMsg));
        return;
    }

    M_A {m_common_data.m_system_name} = pRspLogin->SystemName;
    M_A {m_common_data.m_user_id}     = pRspLogin->UserID;
    m_common_data.m_front_id          = pRspLogin->FrontID;
    m_common_data.m_session_id        = pRspLogin->SessionID;

    ticker_taper.reset(static_cast<int>(pRspLogin->PrivateSeq));

    logger->info("Logged to {} successfully as UserID {} with FrontID/SessionID {}/{} at {}-{}", pRspLogin->SystemName, pRspLogin->UserID, pRspLogin->FrontID, pRspLogin->SessionID, pRspLogin->TradingDay, pRspLogin->LoginTime);

    m_parent->notify_login_success();

    /// Initial queries.
    init_req();
}

void trade::broker::CUTTraderImpl::OnFrontDisconnected(int nReason)
{
    CUTSpi::OnFrontDisconnected(nReason);

    logger->info("Logged out successfully from {} as UserID {} with FrontID/SessionID {}/{}", m_common_data.m_system_name, m_common_data.m_user_id, m_common_data.m_front_id, m_common_data.m_session_id);

    m_parent->notify_logout_success();
}

/// @ReqQryInvestor.
void trade::broker::CUTTraderImpl::OnRspQryInvestor(
    CUTInvestorField* pInvestor,
    CUTRspInfoField* pRspInfo,
    int nRequestID, bool bIsLast
)
{
    CUTSpi::OnRspQryInvestor(pInvestor, pRspInfo, nRequestID, bIsLast);

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

/// @ReqQryInstrument.
void trade::broker::CUTTraderImpl::OnRspQryInstrument(
    CUTInstrumentField* pInstrument,
    CUTRspInfoField* pRspInfo,
    const int nRequestID, const bool bIsLast
)
{
    CUTSpi::OnRspQryInstrument(pInstrument, pRspInfo, nRequestID, bIsLast);

    NULLPTR_CHECKER(pRspInfo);

    const auto request_id = get_by_seq_id(nRequestID);

    if (pRspInfo->ErrorID != 0) {
        logger->error("Failed to load instrument for request {}: {}", request_id.value_or(INVALID_ID), pRspInfo->ErrorMsg);
        return;
    }

    logger->debug("Loaded instrument {}({}) whose underlying is {}", utilities::GB2312ToUTF8()(pInstrument->InstrumentName), pInstrument->InstrumentID, pInstrument->UnderlyingInstrID);

    /// For caching between multiple calls.
    static std::unordered_map<decltype(snow_flaker()), std::shared_ptr<types::Symbols>> symbol_cache;

    const auto symbols = symbol_cache.emplace(request_id.value_or(INVALID_ID), std::make_shared<types::Symbols>()).first->second;

    types::Symbol symbol;

    symbol.set_symbol(pInstrument->InstrumentID);
    symbol.set_symbol_name(utilities::GB2312ToUTF8()(pInstrument->InstrumentName));
    symbol.set_exchange(CUTCommonData::to_exchange(pInstrument->ExchangeID));
    if (!std::string(pInstrument->UnderlyingInstrID).empty())
        symbol.set_underlying(pInstrument->UnderlyingInstrID);

    symbols->add_symbols()->CopyFrom(symbol);

    if (bIsLast) {
        logger->info("Loaded {} instruments for request {}", symbols->symbols_size(), request_id.value_or(INVALID_ID));
        m_holder->update_symbols(symbols);
        symbol_cache.erase(request_id.value_or(INVALID_ID));
    }
}

/// @ReqQryTradingAccount.
void trade::broker::CUTTraderImpl::OnRspQryTradingAccount(
    CUTTradingAccountField* pTradingAccount,
    CUTRspInfoField* pRspInfo,
    const int nRequestID, const bool bIsLast
)
{
    CUTSpi::OnRspQryTradingAccount(pTradingAccount, pRspInfo, nRequestID, bIsLast);

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
        logger->info("Loaded {} trading accounts for request {}", funds->funds_size(), request_id.value_or(INVALID_ID));
        m_holder->update_funds(funds);
        fund_cache.erase(request_id.value_or(INVALID_ID));
    }
}

/// @ReqQryInvestorPosition.
void trade::broker::CUTTraderImpl::OnRspQryInvestorPosition(
    CUTInvestorPositionField* pInvestorPosition,
    CUTRspInfoField* pRspInfo,
    const int nRequestID, const bool bIsLast
)
{
    CUTSpi::OnRspQryInvestorPosition(pInvestorPosition, pRspInfo, nRequestID, bIsLast);

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
    position.set_pre_margin(pInvestorPosition->UseMargin); /// TODO: Find a way to get PreMargin(上次占用的保证金) here.
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
void trade::broker::CUTTraderImpl::OnRspOrderInsert(
    CUTInputOrderField* pInputOrder,
    CUTRspInfoField* pRspInfo,
    const int nRequestID, const bool bIsLast
)
{
    CUTSpi::OnRspOrderInsert(pInputOrder, pRspInfo, nRequestID, bIsLast);

    NULLPTR_CHECKER(pRspInfo);

    const auto unique_id = get_by_seq_id(nRequestID);

    /// This callback is called only when an order is rejected by front end.
    /// Original description:
    /// 正确报单不返回。
    assert(pRspInfo->ErrorID != 0);

    const auto order_rejection = std::make_shared<types::OrderRejection>();

    order_rejection->set_original_unique_id(unique_id.value_or(INVALID_ID));
    if (pInputOrder != nullptr) {
        order_rejection->set_original_broker_id(CUTCommonData::to_broker_id(m_common_data.m_front_id, m_common_data.m_session_id, pInputOrder->OrderRef));
        order_rejection->set_original_exchange_id(CUTCommonData::to_exchange_id(pInputOrder->ExchangeID, "")); /// No OrderSysRef here.
    }
    order_rejection->set_rejection_code(types::RejectionCode::unknown);
    order_rejection->set_rejection_reason(utilities::GB2312ToUTF8()(pRspInfo->ErrorMsg));
    order_rejection->set_allocated_rejection_time(utilities::Now<google::protobuf::Timestamp*>()());

    m_reporter->order_rejected(order_rejection);
}

void trade::broker::CUTTraderImpl::OnRspOrderAction(
    CUTInputOrderActionField* pInputOrderAction,
    CUTRspInfoField* pRspInfo,
    const int nRequestID, const bool bIsLast
)
{
    CUTSpi::OnRspOrderAction(pInputOrderAction, pRspInfo, nRequestID, bIsLast);

    NULLPTR_CHECKER(pRspInfo);

    const auto request_id = get_by_seq_id(nRequestID);

    /// This callback is called only when an order action is rejected by front end.
    /// Original description:
    /// 正确报单不返回。
    assert(pRspInfo->ErrorID != 0);

    const auto cancel_order_rejection = std::make_shared<types::CancelOrderRejection>();

    cancel_order_rejection->set_original_unique_id(request_id.value_or(INVALID_ID));
    if (pInputOrderAction != nullptr) {
        cancel_order_rejection->set_original_broker_id(CUTCommonData::to_broker_id(pInputOrderAction->FrontID, pInputOrderAction->SessionID, pInputOrderAction->OrderRef));
        cancel_order_rejection->set_original_exchange_id(CUTCommonData::to_exchange_id(pInputOrderAction->ExchangeID, "")); /// No OrderSysRef here.
    }
    cancel_order_rejection->set_rejection_code(types::RejectionCode::unknown);
    cancel_order_rejection->set_rejection_reason(utilities::GB2312ToUTF8()(pRspInfo->ErrorMsg));
    cancel_order_rejection->set_allocated_rejection_time(utilities::Now<google::protobuf::Timestamp*>()());

    m_reporter->cancel_order_rejected(cancel_order_rejection);
}

void trade::broker::CUTTraderImpl::OnErrRtnOrderAction(CUTOrderActionField* pOrderAction)
{
    CUTSpi::OnErrRtnOrderAction(pOrderAction);

    NULLPTR_CHECKER(pOrderAction);

    /// TODO: Convert ExchangeErrorID to explanatory string.
    logger->warn("An outside cancel {} from ip {} mac {} is rejected by exchange: {}", CUTCommonData::to_exchange_id(pOrderAction->ExchangeID, pOrderAction->OrderSysID), pOrderAction->IPAddressAsInt, pOrderAction->MacAddressAsLong, pOrderAction->ExchangeErrorID);
}

void trade::broker::CUTTraderImpl::OnRtnOrder(CUTOrderField* pOrder)
{
    CUTSpi::OnRtnOrder(pOrder);

    NULLPTR_CHECKER(pOrder);

    logger->debug("New CUTOrder {} arrived with status {}", CUTCommonData::to_exchange_id(pOrder->ExchangeID, pOrder->OrderSysID), pOrder->OrderStatus);

    using OrderRefType = std::tuple_element_t<0, decltype(new_id_pair())>;
    OrderRefType order_ref;

    order_ref      = pOrder->OrderRef;

    auto unique_id = get_by_seq_id(order_ref);

    std::shared_ptr<types::Orders> orders;
    types::Order order;

    /// No unique_id in AppBase means that the order not submitted by this
    /// instance.
    if (!unique_id.has_value()) {
        /// Check if the order is already in holder.
        orders = m_holder->query_orders_by_exchange_id(CUTCommonData::to_exchange_id(pOrder->ExchangeID, pOrder->OrderSysID));
        if (orders->orders().empty()) {
            /// Insert the new outside order into holder.
            order.set_unique_id(snow_flaker());
            order.set_broker_id(CUTCommonData::to_broker_id(m_common_data.m_front_id, m_common_data.m_session_id, pOrder->OrderRef));
            order.set_exchange_id(CUTCommonData::to_exchange_id(pOrder->ExchangeID, pOrder->OrderSysID));
            order.set_symbol(pOrder->InstrumentID);
            order.set_side(CUTCommonData::to_side(pOrder->Direction));
            order.set_position_side(CUTCommonData::to_position_side(pOrder->OffsetFlag));
            order.set_price(pOrder->LimitPrice);
            order.set_quantity(pOrder->VolumeTotalOriginal);
            order.set_allocated_creation_time(utilities::Now<google::protobuf::Timestamp*>()());

            orders->add_orders()->CopyFrom(order);

            m_holder->update_orders(orders);

            logger->info("Assigned new unique_id {} to new outside order {} from ip {} mac {} and inserted into holder", order.unique_id(), CUTCommonData::to_exchange_id(pOrder->ExchangeID, pOrder->OrderSysID), pOrder->IPAddressAsInt, pOrder->MacAddressAsLong);
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
            order.set_broker_id(CUTCommonData::to_broker_id(pOrder->FrontID, pOrder->SessionID, pOrder->OrderRef));
            order.set_exchange_id(CUTCommonData::to_exchange_id(pOrder->ExchangeID, pOrder->OrderSysID));
            order.set_allocated_update_time(utilities::Now<google::protobuf::Timestamp*>()());

            m_holder->update_orders(orders);
        }
    }

    switch (pOrder->OrderStatus) {
    case UT_OST_Canceled: {
        const auto order_rejection = std::make_shared<types::OrderRejection>();

        order_rejection->set_original_unique_id(unique_id.value());
        order_rejection->set_original_broker_id(CUTCommonData::to_broker_id(pOrder->FrontID, pOrder->SessionID, pOrder->OrderRef));
        order_rejection->set_original_exchange_id(CUTCommonData::to_broker_id(pOrder->FrontID, pOrder->SessionID, pOrder->OrderRef));
        order_rejection->set_rejection_code(types::RejectionCode::unknown);
        order_rejection->set_rejection_reason(fmt::format("ExchangeErrorID {}", pOrder->ExchangeErrorID)); /// TODO: Convert ExchangeErrorID to explanatory string.
        order_rejection->set_allocated_rejection_time(utilities::Now<google::protobuf::Timestamp*>()());

        m_reporter->order_rejected(order_rejection);

        break;
    }
    default: break;
    }
}
