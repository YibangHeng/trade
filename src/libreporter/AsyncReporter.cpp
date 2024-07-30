#include "libreporter/AsyncReporter.h"

trade::reporter::AsyncReporter::AsyncReporter(std::shared_ptr<IReporter> outside)
    : m_outside(std::move(outside))
{
    m_is_running = true;

    /// Order.
    m_broker_acceptance_thread   = std::thread(&AsyncReporter::do_broker_accepted, this);
    m_exchange_acceptance_thread = std::thread(&AsyncReporter::do_exchange_accepted, this);
    m_order_rejection_thread     = std::thread(&AsyncReporter::do_order_rejected, this);

    /// Cancel.
    m_cancel_broker_acceptance_thread   = std::thread(&AsyncReporter::do_cancel_broker_accepted, this);
    m_cancel_exchange_acceptance_thread = std::thread(&AsyncReporter::do_cancel_exchange_accepted, this);
    m_cancel_success_thread             = std::thread(&AsyncReporter::do_cancel_success, this);
    m_cancel_order_rejection_thread     = std::thread(&AsyncReporter::do_cancel_order_rejected, this);

    /// Trade.
    m_trade_thread = std::thread(&AsyncReporter::do_trade_accepted, this);

    /// Market data.
    m_exchange_order_tick_thread = std::thread(&AsyncReporter::do_exchange_order_tick_arrived, this);
    m_exchange_trade_tick_thread = std::thread(&AsyncReporter::do_exchange_trade_tick_arrived, this);
    m_exchange_l2_tick_thread    = std::thread(&AsyncReporter::do_exchange_l2_tick_arrived, this);
    m_l2_tick_thread             = std::thread(&AsyncReporter::do_l2_tick_generated, this);
}

trade::reporter::AsyncReporter::~AsyncReporter()
{
    m_is_running = false;

    /// Order.
    m_broker_acceptance_thread.join();
    m_exchange_acceptance_thread.join();
    m_order_rejection_thread.join();

    /// Cancel.
    m_cancel_broker_acceptance_thread.join();
    m_cancel_exchange_acceptance_thread.join();
    m_cancel_success_thread.join();
    m_cancel_order_rejection_thread.join();

    /// Trade.
    m_trade_thread.join();

    /// Market data.
    m_exchange_order_tick_thread.join();
    m_exchange_trade_tick_thread.join();
    m_exchange_l2_tick_thread.join();
    m_l2_tick_thread.join();
}

void trade::reporter::AsyncReporter::broker_accepted(const std::shared_ptr<types::BrokerAcceptance> broker_acceptance)
{
    while (!m_broker_acceptance_buffer.push(broker_acceptance))
        ;
}

void trade::reporter::AsyncReporter::exchange_accepted(const std::shared_ptr<types::ExchangeAcceptance> exchange_acceptance)
{
    while (!m_exchange_acceptance_buffer.push(exchange_acceptance))
        ;
}

void trade::reporter::AsyncReporter::order_rejected(const std::shared_ptr<types::OrderRejection> order_rejection)
{
    while (!m_order_rejection_buffer.push(order_rejection))
        ;
}

void trade::reporter::AsyncReporter::cancel_broker_accepted(const std::shared_ptr<types::CancelBrokerAcceptance> cancel_broker_acceptance)
{
    while (!m_cancel_broker_acceptance_buffer.push(cancel_broker_acceptance))
        ;
}

void trade::reporter::AsyncReporter::cancel_exchange_accepted(const std::shared_ptr<types::CancelExchangeAcceptance> cancel_exchange_acceptance)
{
    while (!m_cancel_exchange_acceptance_buffer.push(cancel_exchange_acceptance))
        ;
}

void trade::reporter::AsyncReporter::cancel_success(const std::shared_ptr<types::CancelSuccess> cancel_success)
{
    while (!m_cancel_success_buffer.push(cancel_success))
        ;
}

void trade::reporter::AsyncReporter::cancel_order_rejected(const std::shared_ptr<types::CancelOrderRejection> cancel_order_rejection)
{
    while (!m_cancel_order_rejection_buffer.push(cancel_order_rejection))
        ;
}

void trade::reporter::AsyncReporter::trade_accepted(const std::shared_ptr<types::Trade> trade)
{
    while (!m_trade_buffer.push(trade))
        ;
}

void trade::reporter::AsyncReporter::exchange_order_tick_arrived(const std::shared_ptr<types::OrderTick> order_tick)
{
    while (!m_exchange_order_tick_buffer.push(order_tick))
        ;
}

void trade::reporter::AsyncReporter::exchange_trade_tick_arrived(const std::shared_ptr<types::TradeTick> trade_tick)
{
    while (!m_exchange_trade_tick_buffer.push(trade_tick))
        ;
}

void trade::reporter::AsyncReporter::exchange_l2_tick_arrived(const std::shared_ptr<types::ExchangeL2Snap> exchange_l2_snap)
{
    while (!m_exchange_l2_snap_buffer.push(exchange_l2_snap))
        ;
}

void trade::reporter::AsyncReporter::l2_tick_generated(const std::shared_ptr<types::GeneratedL2Tick> generated_l2_tick)
{
    while (!m_generated_l2_tick_buffer.push(generated_l2_tick))
        ;
}

void trade::reporter::AsyncReporter::do_broker_accepted()
{
    while (m_is_running || !m_broker_acceptance_buffer.empty()) {
        std::shared_ptr<types::BrokerAcceptance> broker_acceptance;

        if (m_broker_acceptance_buffer.pop(broker_acceptance))
            m_outside->broker_accepted(broker_acceptance);
    }
}

void trade::reporter::AsyncReporter::do_exchange_accepted()
{
    while (m_is_running || !m_exchange_acceptance_buffer.empty()) {
        std::shared_ptr<types::ExchangeAcceptance> exchange_acceptance;

        if (m_exchange_acceptance_buffer.pop(exchange_acceptance))
            m_outside->exchange_accepted(exchange_acceptance);
    }
}

void trade::reporter::AsyncReporter::do_order_rejected()
{
    while (m_is_running || !m_order_rejection_buffer.empty()) {
        std::shared_ptr<types::OrderRejection> order_rejection;

        if (m_order_rejection_buffer.pop(order_rejection))
            m_outside->order_rejected(order_rejection);
    }
}

void trade::reporter::AsyncReporter::do_cancel_broker_accepted()
{
    while (m_is_running || !m_cancel_broker_acceptance_buffer.empty()) {
        std::shared_ptr<types::CancelBrokerAcceptance> cancel_broker_acceptance;

        if (m_cancel_broker_acceptance_buffer.pop(cancel_broker_acceptance))
            m_outside->cancel_broker_accepted(cancel_broker_acceptance);
    }
}

void trade::reporter::AsyncReporter::do_cancel_exchange_accepted()
{
    while (m_is_running || !m_cancel_exchange_acceptance_buffer.empty()) {
        std::shared_ptr<types::CancelExchangeAcceptance> cancel_exchange_acceptance;

        if (m_cancel_exchange_acceptance_buffer.pop(cancel_exchange_acceptance))
            m_outside->cancel_exchange_accepted(cancel_exchange_acceptance);
    }
}

void trade::reporter::AsyncReporter::do_cancel_success()
{
    while (m_is_running || !m_cancel_success_buffer.empty()) {
        std::shared_ptr<types::CancelSuccess> cancel_success;

        if (m_cancel_success_buffer.pop(cancel_success))
            m_outside->cancel_success(cancel_success);
    }
}

void trade::reporter::AsyncReporter::do_cancel_order_rejected()
{
    while (m_is_running || !m_cancel_order_rejection_buffer.empty()) {
        std::shared_ptr<types::CancelOrderRejection> cancel_order_rejection;

        if (m_cancel_order_rejection_buffer.pop(cancel_order_rejection))
            m_outside->cancel_order_rejected(cancel_order_rejection);
    }
}

void trade::reporter::AsyncReporter::do_trade_accepted()
{
    while (m_is_running || !m_trade_buffer.empty()) {
        std::shared_ptr<types::Trade> trade;

        if (m_trade_buffer.pop(trade))
            m_outside->trade_accepted(trade);
    }
}

void trade::reporter::AsyncReporter::do_exchange_order_tick_arrived()
{
    while (m_is_running || !m_exchange_order_tick_buffer.empty()) {
        std::shared_ptr<types::OrderTick> exchange_order_tick;

        if (m_exchange_order_tick_buffer.pop(exchange_order_tick)) {
            m_outside->exchange_order_tick_arrived(exchange_order_tick);
        }
    }
}

void trade::reporter::AsyncReporter::do_exchange_trade_tick_arrived()
{
    while (m_is_running || !m_exchange_trade_tick_buffer.empty()) {
        std::shared_ptr<types::TradeTick> exchange_trade_tick;

        if (m_exchange_trade_tick_buffer.pop(exchange_trade_tick))
            m_outside->exchange_trade_tick_arrived(exchange_trade_tick);
    }
}

void trade::reporter::AsyncReporter::do_exchange_l2_tick_arrived()
{
    while (m_is_running || !m_exchange_l2_snap_buffer.empty()) {
        std::shared_ptr<types::ExchangeL2Snap> exchange_l2_tick;

        if (m_exchange_l2_snap_buffer.pop(exchange_l2_tick))
            m_outside->exchange_l2_tick_arrived(exchange_l2_tick);
    }
}

void trade::reporter::AsyncReporter::do_l2_tick_generated()
{
    while (m_is_running || !m_generated_l2_tick_buffer.empty()) {
        std::shared_ptr<types::GeneratedL2Tick> generated_l2_tick;

        if (m_generated_l2_tick_buffer.pop(generated_l2_tick))
            m_outside->l2_tick_generated(generated_l2_tick);
    }
}
