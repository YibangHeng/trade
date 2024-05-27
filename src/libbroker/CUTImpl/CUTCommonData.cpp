#include <fmt/format.h>
#include <regex>

#include "libbooker/BookerCommonData.h"
#include "libbroker/CUTImpl/CUTCommonData.h"
#include "utilities/TimeHelper.hpp"

trade::broker::CUTCommonData::CUTCommonData()
    : m_system_name(),
      m_user_id(),
      m_investor_id(),
      m_front_id(0),
      m_session_id(0)
{}

TUTExchangeIDType trade::broker::CUTCommonData::to_exchange(const types::ExchangeType exchange)
{
    switch (exchange) {
    case types::ExchangeType::bse: return UT_EXG_BSE;
    case types::ExchangeType::cffex: return UT_EXG_CFFEX;
    case types::ExchangeType::czce: return UT_EXG_CZCE;
    case types::ExchangeType::dce: return UT_EXG_DCE;
    case types::ExchangeType::hkex: return UT_EXG_HKEX;
    case types::ExchangeType::ine: return UT_EXG_INE;
    case types::ExchangeType::shfe: return UT_EXG_SHFE;
    case types::ExchangeType::sse: return UT_EXG_SSE;
    case types::ExchangeType::szse: return UT_EXG_SZSE;
    default: return UT_EXG_OTHER;
    }
}

trade::types::ExchangeType trade::broker::CUTCommonData::to_exchange(const TUTExchangeIDType exchange)
{
    switch (exchange) {
    case UT_EXG_BSE: return types::ExchangeType::bse;
    case UT_EXG_CFFEX: return types::ExchangeType::cffex;
    case UT_EXG_CZCE: return types::ExchangeType::czce;
    case UT_EXG_DCE: return types::ExchangeType::dce;
    case UT_EXG_HKEX: return types::ExchangeType::hkex;
    case UT_EXG_INE: return types::ExchangeType::ine;
    case UT_EXG_SHFE: return types::ExchangeType::shfe;
    case UT_EXG_SSE: return types::ExchangeType::sse;
    case UT_EXG_SZSE: return types::ExchangeType::szse;
    default: return types::ExchangeType::invalid_exchange;
    }
}

trade::types::OrderType trade::broker::CUTCommonData::to_order_type(const char order_type)
{
    switch (order_type) {
    case '2': return types::OrderType::limit;
    case '1': return types::OrderType::market;
    case 'U': return types::OrderType::best_price;
    case '4': return types::OrderType::cancel;
    default: return types::OrderType::invalid_order_type;
    }
}

TUTDirectionType trade::broker::CUTCommonData::to_side(const types::SideType side)
{
    switch (side) {
    case types::SideType::buy: return UT_D_Buy;
    case types::SideType::sell: return UT_D_Sell;
    default: return '\0';
    }
}

trade::types::SideType trade::broker::CUTCommonData::to_side(const TUTDirectionType side)
{
    switch (side) {
    case UT_D_Buy: return types::SideType::buy;
    case UT_D_Sell: return types::SideType::sell;
    default: return types::SideType::invalid_side;
    }
}

/// TODO: Confirmation is required.
trade::types::SideType trade::broker::CUTCommonData::to_md_side(const char side)
{
    switch (side) {
    case '1': return types::SideType::buy;
    case '2': return types::SideType::sell;
    default: return types::SideType::invalid_side;
    }
}

TUTOffsetFlagType trade::broker::CUTCommonData::to_position_side(const types::PositionSideType position_side)
{
    switch (position_side) {
    case types::PositionSideType::open: return UT_OF_Open;
    case types::PositionSideType::close: return UT_OF_Close;
    case types::PositionSideType::force_close: return UT_OF_ForceClose;
    case types::PositionSideType::close_today: return UT_OF_CloseToday;
    case types::PositionSideType::close_yesterday: return UT_OF_CloseYesterday;
    case types::PositionSideType::force_off: return UT_OF_ForceOff;
    case types::PositionSideType::local_force_close: return UT_OF_LocalForceClose;
    default: return '\0';
    }
}

trade::types::PositionSideType trade::broker::CUTCommonData::to_position_side(const TUTOffsetFlagType position_side)
{
    switch (position_side) {
    case UT_OF_Open: return types::PositionSideType::open;
    case UT_OF_Close: return types::PositionSideType::close;
    case UT_OF_ForceClose: return types::PositionSideType::force_close;
    case UT_OF_CloseToday: return types::PositionSideType::close_today;
    case UT_OF_CloseYesterday: return types::PositionSideType::close_yesterday;
    case UT_OF_ForceOff: return types::PositionSideType::force_off;
    case UT_OF_LocalForceClose: return types::PositionSideType::local_force_close;
    default: return types::PositionSideType::invalid_position_side;
    }
}

std::string trade::broker::CUTCommonData::to_broker_id(
    TUTFrontIDType front_id,
    TUTSessionIDType session_id,
    const int order_ref
)
{
    return fmt::format("{}:{}:{}", front_id, session_id, order_ref);
}

std::tuple<std::string, std::string, std::string> trade::broker::CUTCommonData::from_broker_id(const std::string& broker_id)
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

std::string trade::broker::CUTCommonData::to_exchange_id(
    const TUTExchangeIDType exchange,
    const std::string& order_sys_id
)
{
    return fmt::format("{}:{}", exchange, order_sys_id);
}

std::tuple<std::string, std::string> trade::broker::CUTCommonData::from_exchange_id(const std::string& exchange_id)
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

trade::types::X_OST_SZSEDatagramType trade::broker::CUTCommonData::get_datagram_type(const std::string& message)
{
    return to_szse_datagram_type(reinterpret_cast<const SZSEHpfPackageHead*>(message.data())->m_message_type);
}

uint8_t trade::broker::CUTCommonData::to_szse_datagram_type(const types::X_OST_SZSEDatagramType message_type)
{
    switch (message_type) {
    case types::X_OST_SZSEDatagramType::order: return 23;
    case types::X_OST_SZSEDatagramType::trade: return 24;
    default: return 0;
    }
}

trade::types::X_OST_SZSEDatagramType trade::broker::CUTCommonData::to_szse_datagram_type(const uint8_t message_type)
{
    switch (message_type) {
    case 23: return types::X_OST_SZSEDatagramType::order;
    case 24: return types::X_OST_SZSEDatagramType::trade;
    default: return types::X_OST_SZSEDatagramType::invalid_ost_szse_datagram_type;
    }
}

std::shared_ptr<trade::types::OrderTick> trade::broker::CUTCommonData::to_order_tick(const std::string& message)
{
    assert(message.size() == sizeof(SZSEHpfOrderTick));

    const auto raw_order  = reinterpret_cast<const SZSEHpfOrderTick*>(message.data());

    const auto order_tick = std::make_shared<types::OrderTick>();

    order_tick->set_symbol(raw_order->m_header.m_symbol);
    order_tick->set_order_type(to_order_type(raw_order->m_order_type));
    order_tick->set_side(to_md_side(raw_order->m_side));
    order_tick->set_price(BookerCommonData::to_price(static_cast<liquibook::book::Price>(raw_order->m_px)));
    order_tick->set_quantity(BookerCommonData::to_quantity(raw_order->m_qty));

    return order_tick;
}

std::shared_ptr<trade::types::TradeTick> trade::broker::CUTCommonData::to_trade_tick(const std::string& message)
{
    assert(message.size() == sizeof(SZSEHpfTradeTick));

    const auto raw_trade  = reinterpret_cast<const SZSEHpfTradeTick*>(message.data());

    const auto trade_tick = std::make_shared<types::TradeTick>();

    trade_tick->set_symbol(raw_trade->m_header.m_symbol);
    trade_tick->set_exec_price(BookerCommonData::to_price(static_cast<liquibook::book::Price>(raw_trade->m_exe_px)));
    trade_tick->set_exec_quantity(BookerCommonData::to_quantity(raw_trade->m_exe_qty));

    return trade_tick;
}
