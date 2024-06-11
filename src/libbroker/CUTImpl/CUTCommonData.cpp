#include <fmt/format.h>
#include <regex>

#include "libbroker/CUTImpl/CUTCommonData.h"
#include "libbroker/CUTImpl/RawStructure.h"
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

trade::types::OrderType trade::broker::CUTCommonData::to_order_type_from_sse(const char tick_type)
{
    switch (tick_type) {
    case 'A': return types::OrderType::limit;
    case 'D': return types::OrderType::cancel;
    default: return types::OrderType::invalid_order_type;
    }
}

trade::types::OrderType trade::broker::CUTCommonData::to_order_type_from_szse(const char order_type)
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

trade::types::SideType trade::broker::CUTCommonData::to_md_side_from_sse(const char side)
{
    switch (side) {
    case 'B': return types::SideType::buy;
    case 'S': return types::SideType::sell;
    default: return types::SideType::invalid_side;
    }
}

trade::types::SideType trade::broker::CUTCommonData::to_md_side_from_szse(const char side)
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

trade::types::ExchangeType trade::broker::CUTCommonData::get_exchange_type(const std::string& message)
{
    /// TODO: Is it OK to check message type by message size?
    switch (message.size()) {
    case sizeof(SSEHpfTick): return types::ExchangeType::sse;
    case sizeof(SZSEHpfOrderTick):
    case sizeof(SZSEHpfTradeTick): return types::ExchangeType::szse;
    default: return types::ExchangeType::invalid_exchange;
    }
}

/// Tick type can be order (including new order and cancel order) or trade.
trade::types::X_OST_TickType trade::broker::CUTCommonData::get_tick_type(const std::string& message, const types::ExchangeType exchange_type)
{
    switch (exchange_type) {
    case types::ExchangeType::sse:
        return to_sse_tick_type(reinterpret_cast<const SSEHpfTick*>(message.data())->m_tick_type);
    case types::ExchangeType::szse:
        return to_szse_tick_type(reinterpret_cast<const SZSEHpfPackageHead*>(message.data())->m_message_type);
    default:
        return types::X_OST_TickType::invalid_ost_datagram_type;
    }
}

char trade::broker::CUTCommonData::to_sse_tick_type(const types::X_OST_TickType message_type)
{
    switch (message_type) {
    case types::X_OST_TickType::order: return 'A';
    case types::X_OST_TickType::trade: return 'T';
    default: return 0;
    }
}

trade::types::X_OST_TickType trade::broker::CUTCommonData::to_sse_tick_type(const char message_type)
{
    /// Original description:
    ///     ……类型，A 新增订单，D 删除订单，S 产品状态订单，T 成交……
    switch (message_type) {
    case 'A':
    case 'D': return types::X_OST_TickType::order;
    case 'T': return types::X_OST_TickType::trade;
    default: return types::X_OST_TickType::invalid_ost_datagram_type;
    }
}

uint8_t trade::broker::CUTCommonData::to_szse_tick_type(const types::X_OST_TickType message_type)
{
    switch (message_type) {
    case types::X_OST_TickType::order: return 23;
    case types::X_OST_TickType::trade: return 24;
    default: return 0;
    }
}

trade::types::X_OST_TickType trade::broker::CUTCommonData::to_szse_tick_type(const uint8_t message_type)
{
    switch (message_type) {
    case 23: return types::X_OST_TickType::order;
    case 24: return types::X_OST_TickType::trade;
    default: return types::X_OST_TickType::invalid_ost_datagram_type;
    }
}

std::shared_ptr<trade::types::OrderTick> trade::broker::CUTCommonData::to_order_tick(
    const std::string& message,
    const types::ExchangeType exchange_type
)
{
    auto order_tick = std::make_shared<types::OrderTick>();

    switch (exchange_type) {
    case types::ExchangeType::sse: {
        assert(message.size() == sizeof(SSEHpfTick));

        const auto raw_order = reinterpret_cast<const SSEHpfTick*>(message.data());

        order_tick->set_unique_id(static_cast<int64_t>(raw_order->m_buy_order_no + raw_order->m_sell_order_no));
        order_tick->set_order_type(to_order_type_from_sse(raw_order->m_tick_type));
        order_tick->set_symbol(raw_order->m_symbol_id);
        order_tick->set_side(to_md_side_from_sse(raw_order->m_side_flag));
        order_tick->set_price(raw_order->m_order_price / 1000.);                 /// TODO: Use convertor instead.
        order_tick->set_quantity(static_cast<int64_t>(raw_order->m_qty) / 1000); /// TODO: Use convertor instead.

        return order_tick;
    }
    case types::ExchangeType::szse: {
        assert(message.size() == sizeof(SZSEHpfOrderTick));

        const auto raw_order = reinterpret_cast<const SZSEHpfOrderTick*>(message.data());

        order_tick->set_unique_id(raw_order->m_header.m_sequence);
        order_tick->set_order_type(to_order_type_from_szse(raw_order->m_order_type));
        order_tick->set_symbol(raw_order->m_header.m_symbol);
        order_tick->set_side(to_md_side_from_szse(raw_order->m_side));
        order_tick->set_price(raw_order->m_px / 100000.);                       /// TODO: Use convertor instead.
        order_tick->set_quantity(static_cast<int64_t>(raw_order->m_qty) / 100); /// TODO: Use convertor instead.

        return order_tick;
    }
    default: {
        return nullptr;
    }
    }
}

std::shared_ptr<trade::types::TradeTick> trade::broker::CUTCommonData::to_trade_tick(const std::string& message, const types::ExchangeType exchange_type)
{
    auto trade_tick = std::make_shared<types::TradeTick>();

    switch (exchange_type) {
    case types::ExchangeType::sse: {
        const auto raw_trade = reinterpret_cast<const SSEHpfTick*>(message.data());

        trade_tick->set_ask_unique_id(static_cast<int64_t>(raw_trade->m_sell_order_no));
        trade_tick->set_bid_unique_id(static_cast<int64_t>(raw_trade->m_buy_order_no));
        trade_tick->set_symbol(raw_trade->m_symbol_id);
        trade_tick->set_exec_price(raw_trade->m_order_price / 1000.);                 /// TODO: Use convertor instead.
        trade_tick->set_exec_quantity(static_cast<int64_t>(raw_trade->m_qty / 1000)); /// TODO: Use convertor instead.

        return trade_tick;
    }
    case types::ExchangeType::szse: {
        assert(message.size() == sizeof(SZSEHpfTradeTick));

        const auto raw_trade = reinterpret_cast<const SZSEHpfTradeTick*>(message.data());

        trade_tick->set_ask_unique_id(raw_trade->m_ask_app_seq_num);
        trade_tick->set_bid_unique_id(raw_trade->m_bid_app_seq_num);
        trade_tick->set_symbol(raw_trade->m_header.m_symbol);
        trade_tick->set_exec_price(raw_trade->m_exe_px / 10000.);                        /// TODO: Use convertor instead.
        trade_tick->set_exec_quantity(static_cast<int64_t>(raw_trade->m_exe_qty / 100)); /// TODO: Use convertor instead.
        trade_tick->set_x_ost_szse_exe_type(to_order_type_from_szse(raw_trade->m_exe_type));

        return trade_tick;
    }
    default: {
        return nullptr;
    }
    }
}
