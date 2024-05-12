#include <google/protobuf/util/time_util.h>
#include <regex>
#include <utility>

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
      m_broker_id(),
      m_user_id(),
      m_investor_id(),
      m_front_id(0),
      m_session_id(0),
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

    M_A {user_logout_field.BrokerID} = m_broker_id;
    M_A {user_logout_field.UserID}   = m_user_id;

    m_login_syncer.start_logout();

    m_md_api->ReqUserLogout(&user_logout_field, ticker_taper());

    /// Wait for @OnRspUserLogout.
    m_login_syncer.wait_logout();

    m_md_api->RegisterSpi(nullptr);
    m_md_api->Release();
}

#define NULLPTR_CHECKER(ptr)                                      \
    if (ptr == nullptr) {                                         \
        logger->warn("A nullptr {} found in {}", #ptr, __func__); \
        return;                                                   \
    }

void trade::broker::CTPMdImpl::OnFrontConnected()
{
    CThostFtdcMdSpi::OnFrontConnected();

    logger->info("Connected to CTP server {} with version {}", config->get<std::string>("Server.TradeAddress"), CThostFtdcMdApi::GetApiVersion());

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
        M_A {m_broker_id} = pRspUserLogin->BrokerID;
        M_A {m_user_id}   = pRspUserLogin->UserID;
        m_front_id        = pRspUserLogin->FrontID;
        m_session_id      = pRspUserLogin->SessionID;

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
        logger->info("Logged out successfully as BrokerID/UserID {}/{} with FrontID/SessionID {}/{}", pUserLogout->BrokerID, pUserLogout->UserID, m_front_id, m_session_id);
    }

    m_login_syncer.notify_logout_success();
}

std::string trade::broker::CTPMdImpl::to_exchange(const types::ExchangeType exchange)
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

trade::types::ExchangeType trade::broker::CTPMdImpl::to_exchange(const std::string& exchange)
{
    if (exchange == "BSE") return types::ExchangeType::bse;
    if (exchange == "CFFEX") return types::ExchangeType::cffex;
    if (exchange == "CME") return types::ExchangeType::cme;
    if (exchange == "CZCE") return types::ExchangeType::czce;
    if (exchange == "DCE") return types::ExchangeType::dce;
    if (exchange == "GFEX") return types::ExchangeType::gfex;
    if (exchange == "HKEX") return types::ExchangeType::hkex;
    if (exchange == "INE") return types::ExchangeType::ine;
    if (exchange == "NASD") return types::ExchangeType::nasd;
    if (exchange == "NYSE") return types::ExchangeType::nyse;
    if (exchange == "SHFE") return types::ExchangeType::shfe;
    if (exchange == "SSE") return types::ExchangeType::sse;
    if (exchange == "SZSE") return types::ExchangeType::szse;
    return types::ExchangeType::invalid_exchange;
}

TThostFtdcDirectionType trade::broker::CTPMdImpl::to_side(const types::SideType side)
{
    switch (side) {
    case types::SideType::buy: return THOST_FTDC_DEN_Buy;
    case types::SideType::sell: return THOST_FTDC_DEN_Sell;
    default: return '\0';
    }
}

trade::types::SideType trade::broker::CTPMdImpl::to_side(const TThostFtdcDirectionType side)
{
    switch (side) {
    case THOST_FTDC_DEN_Buy: return types::SideType::buy;
    case THOST_FTDC_DEN_Sell: return types::SideType::sell;
    default: return types::SideType::invalid_side;
    }
}

char trade::broker::CTPMdImpl::to_position_side(const types::PositionSideType position_side)
{
    switch (position_side) {
    case types::PositionSideType::open: return THOST_FTDC_OFEN_Open;
    case types::PositionSideType::close: return THOST_FTDC_OFEN_Close;
    default: return '\0';
    }
}

trade::types::PositionSideType trade::broker::CTPMdImpl::to_position_side(const char position_side)
{
    switch (position_side) {
    case THOST_FTDC_OFEN_Open: return types::PositionSideType::open;
    case THOST_FTDC_OFEN_Close: return types::PositionSideType::close;
    default: return types::PositionSideType::invalid_position_side;
    }
}

std::string trade::broker::CTPMdImpl::to_broker_id(
    TThostFtdcFrontIDType front_id,
    TThostFtdcSessionIDType session_id,
    const std::string& order_ref
)
{
    return fmt::format("{}:{}:{}", front_id, session_id, order_ref);
}

std::tuple<std::string, std::string, std::string> trade::broker::CTPMdImpl::from_broker_id(const std::string& broker_id)
{
    std::string front_id, session_id, order_ref;

    static std::regex re("^([^:]+):([^:]+):([^:]+)$");
    if (!std::regex_match(broker_id, re)) {
        return std::make_tuple(std::string(""), std::string(""), std::string(""));
    }

    std::istringstream iss(broker_id);
    std::getline(iss, front_id, ':');
    std::getline(iss, session_id, ':');
    std::getline(iss, order_ref, ':');

    return std::make_tuple(front_id, session_id, order_ref);
}

std::string trade::broker::CTPMdImpl::to_exchange_id(
    const std::string& exchange,
    const std::string& order_sys_id
)
{
    return fmt::format("{}:{}", exchange, order_sys_id);
}

std::tuple<std::string, std::string> trade::broker::CTPMdImpl::from_exchange_id(const std::string& exchange_id)
{
    std::string exchange, order_sys_id;

    static std::regex re("^([^:]+):([^:]+)$");
    if (!std::regex_match(exchange_id, re)) {
        return std::make_tuple(std::string(""), std::string(""));
    }

    std::istringstream iss(exchange_id);
    std::getline(iss, exchange, ':');
    std::getline(iss, order_sys_id, ':');

    return std::make_tuple(exchange, order_sys_id);
}
