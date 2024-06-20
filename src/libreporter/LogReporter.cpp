#include <fmt/ranges.h>

#include "libreporter/LogReporter.h"
#include "utilities/ToJSON.hpp"

void trade::reporter::LogReporter::broker_accepted(const std::shared_ptr<types::BrokerAcceptance> broker_acceptance)
{
    trade_logger->info("Broker accepted: {}", utilities::ToJSON()(*broker_acceptance));

    m_outside->broker_accepted(broker_acceptance);
}

void trade::reporter::LogReporter::exchange_accepted(const std::shared_ptr<types::ExchangeAcceptance> exchange_acceptance)
{
    trade_logger->info("Exchange accepted: {}", utilities::ToJSON()(*exchange_acceptance));

    m_outside->exchange_accepted(exchange_acceptance);
}

void trade::reporter::LogReporter::order_rejected(const std::shared_ptr<types::OrderRejection> order_rejection)
{
    trade_logger->info("Order rejected: {}", utilities::ToJSON()(*order_rejection));

    m_outside->order_rejected(order_rejection);
}

void trade::reporter::LogReporter::cancel_broker_accepted(const std::shared_ptr<types::CancelBrokerAcceptance> cancel_broker_acceptance)
{
    trade_logger->info("Cancel broker accepted: {}", utilities::ToJSON()(*cancel_broker_acceptance));

    m_outside->cancel_broker_accepted(cancel_broker_acceptance);
}

void trade::reporter::LogReporter::cancel_exchange_accepted(const std::shared_ptr<types::CancelExchangeAcceptance> cancel_exchange_acceptance)
{
    trade_logger->info("Cancel exchange accepted: {}", utilities::ToJSON()(*cancel_exchange_acceptance));

    m_outside->cancel_exchange_accepted(cancel_exchange_acceptance);
}

void trade::reporter::LogReporter::cancel_success(const std::shared_ptr<types::CancelSuccess> cancel_success)
{
    trade_logger->info("Cancel success: {}", utilities::ToJSON()(*cancel_success));

    m_outside->cancel_success(cancel_success);
}

void trade::reporter::LogReporter::cancel_order_rejected(const std::shared_ptr<types::CancelOrderRejection> cancel_order_rejection)
{
    trade_logger->info("Cancel order rejected: {}", utilities::ToJSON()(*cancel_order_rejection));

    m_outside->cancel_order_rejected(cancel_order_rejection);
}

void trade::reporter::LogReporter::trade_accepted(const std::shared_ptr<types::Trade> trade)
{
    trade_logger->info("Trade accepted: {}", utilities::ToJSON()(*trade));

    m_outside->trade_accepted(trade);
}

void trade::reporter::LogReporter::exchange_l2_tick_arrived(std::shared_ptr<types::L2Tick> l2_tick)
{
    md_logger->debug("Exchange L2 tick arrived: {}", utilities::ToJSON()(*l2_tick));

    m_outside->exchange_l2_tick_arrived(l2_tick);
}

void trade::reporter::LogReporter::l2_tick_generated(const std::shared_ptr<types::L2Tick> l2_tick)
{
    md_logger->debug("L2 tick generated: {}", utilities::ToJSON()(*l2_tick));

    m_outside->l2_tick_generated(l2_tick);
}
