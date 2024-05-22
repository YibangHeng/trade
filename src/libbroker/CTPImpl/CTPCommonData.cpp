#include <fmt/format.h>
#include <regex>

#include "libbroker/CTPImpl/CTPCommonData.h"
#include "utilities/TimeHelper.hpp"

trade::broker::CTPCommonData::CTPCommonData()
    : m_system_name(),
      m_broker_id(),
      m_user_id(),
      m_investor_id(),
      m_front_id(0),
      m_session_id(0)
{
}

std::string trade::broker::CTPCommonData::to_exchange(const types::ExchangeType exchange)
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

trade::types::ExchangeType trade::broker::CTPCommonData::to_exchange(const std::string& exchange)
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

TThostFtdcDirectionType trade::broker::CTPCommonData::to_side(const types::SideType side)
{
    switch (side) {
    case types::SideType::buy: return THOST_FTDC_DEN_Buy;
    case types::SideType::sell: return THOST_FTDC_DEN_Sell;
    default: return '\0';
    }
}

trade::types::SideType trade::broker::CTPCommonData::to_side(const TThostFtdcDirectionType side)
{
    switch (side) {
    case THOST_FTDC_DEN_Buy: return types::SideType::buy;
    case THOST_FTDC_DEN_Sell: return types::SideType::sell;
    default: return types::SideType::invalid_side;
    }
}

char trade::broker::CTPCommonData::to_position_side(const types::PositionSideType position_side)
{
    switch (position_side) {
    case types::PositionSideType::open: return THOST_FTDC_OFEN_Open;
    case types::PositionSideType::close: return THOST_FTDC_OFEN_Close;
    default: return '\0';
    }
}

trade::types::PositionSideType trade::broker::CTPCommonData::to_position_side(const char position_side)
{
    switch (position_side) {
    case THOST_FTDC_OFEN_Open: return types::PositionSideType::open;
    case THOST_FTDC_OFEN_Close: return types::PositionSideType::close;
    default: return types::PositionSideType::invalid_position_side;
    }
}

std::string trade::broker::CTPCommonData::to_broker_id(
    TThostFtdcFrontIDType front_id,
    TThostFtdcSessionIDType session_id,
    const std::string& order_ref
)
{
    return fmt::format("{}:{}:{}", front_id, session_id, order_ref);
}

std::tuple<std::string, std::string, std::string> trade::broker::CTPCommonData::from_broker_id(const std::string& broker_id)
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

std::string trade::broker::CTPCommonData::to_exchange_id(
    const std::string& exchange,
    const std::string& order_sys_id
)
{
    return fmt::format("{}:{}", exchange, order_sys_id);
}

std::tuple<std::string, std::string> trade::broker::CTPCommonData::from_exchange_id(const std::string& exchange_id)
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
