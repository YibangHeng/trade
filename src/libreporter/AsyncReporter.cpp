#include "libreporter/AsyncReporter.h"

trade::reporter::AsyncReporter::AsyncReporter(std::shared_ptr<IReporter> outside)
    : /// Order.
      m_broker_acceptance_buffer(m_buffer_size),
      m_exchange_acceptance_buffer(m_buffer_size),
      m_order_rejection_buffer(m_buffer_size),
      /// Cancel.
      m_cancel_broker_acceptance_buffer(m_buffer_size),
      m_cancel_exchange_acceptance_buffer(m_buffer_size),
      m_cancel_success_buffer(m_buffer_size),
      m_cancel_order_rejection_buffer(m_buffer_size),
      /// Trade.
      m_trade_buffer(m_buffer_size),
      /// Market data.
      m_md_trade_buffer(m_buffer_size),
      m_outside(std::move(outside))
{
    is_running = true;

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
    m_md_trade_thread = std::thread(&AsyncReporter::do_md_trade_generated, this);
}

trade::reporter::AsyncReporter::~AsyncReporter()
{
    is_running = false;

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
    m_md_trade_thread.join();
}

void trade::reporter::AsyncReporter::broker_accepted(const std::shared_ptr<types::BrokerAcceptance> broker_acceptance)
{
    std::lock_guard lock(m_broker_acceptance_mutex);
    m_broker_acceptance_buffer.push_back(broker_acceptance);
}

void trade::reporter::AsyncReporter::exchange_accepted(const std::shared_ptr<types::ExchangeAcceptance> exchange_acceptance)
{
    std::lock_guard lock(m_exchange_acceptance_mutex);
    m_exchange_acceptance_buffer.push_back(exchange_acceptance);
}

void trade::reporter::AsyncReporter::order_rejected(const std::shared_ptr<types::OrderRejection> order_rejection)
{
    std::lock_guard lock(m_order_rejection_mutex);
    m_order_rejection_buffer.push_back(order_rejection);
}

void trade::reporter::AsyncReporter::cancel_broker_accepted(const std::shared_ptr<types::CancelBrokerAcceptance> cancel_broker_acceptance)
{
    std::lock_guard lock(m_cancel_broker_acceptance_mutex);
    m_cancel_broker_acceptance_buffer.push_back(cancel_broker_acceptance);
}

void trade::reporter::AsyncReporter::cancel_exchange_accepted(const std::shared_ptr<types::CancelExchangeAcceptance> cancel_exchange_acceptance)
{
    std::lock_guard lock(m_cancel_exchange_acceptance_mutex);
    m_cancel_exchange_acceptance_buffer.push_back(cancel_exchange_acceptance);
}

void trade::reporter::AsyncReporter::cancel_success(const std::shared_ptr<types::CancelSuccess> cancel_success)
{
    std::lock_guard lock(m_cancel_success_mutex);
    m_cancel_success_buffer.push_back(cancel_success);
}

void trade::reporter::AsyncReporter::cancel_order_rejected(const std::shared_ptr<types::CancelOrderRejection> cancel_order_rejection)
{
    std::lock_guard lock(m_cancel_order_rejection_mutex);
    m_cancel_order_rejection_buffer.push_back(cancel_order_rejection);
}

void trade::reporter::AsyncReporter::trade_accepted(const std::shared_ptr<types::Trade> trade)
{
    std::lock_guard lock(m_trade_mutex);
    m_trade_buffer.push_back(trade);
}

void trade::reporter::AsyncReporter::md_trade_generated(const std::shared_ptr<types::MdTrade> md_trade)
{
    std::lock_guard lock(m_md_trade_mutex);
    m_md_trade_buffer.push_back(md_trade);
}

#define IS_RUNNING(buffer, mutex)               \
    [this] {                                    \
        std::lock_guard lock(mutex);            \
        return is_running || !(buffer).empty(); \
    }()

void trade::reporter::AsyncReporter::do_broker_accepted()
{
    while (IS_RUNNING(m_broker_acceptance_buffer, m_broker_acceptance_mutex)) {
        std::shared_ptr<types::BrokerAcceptance> broker_acceptance;
        {
            std::lock_guard lock(m_broker_acceptance_mutex);

            if (m_broker_acceptance_buffer.empty())
                continue;

            broker_acceptance = m_broker_acceptance_buffer[0];
            m_broker_acceptance_buffer.pop_front();
        }
        m_outside->broker_accepted(broker_acceptance);
    }
}

void trade::reporter::AsyncReporter::do_exchange_accepted()
{
    while (IS_RUNNING(m_exchange_acceptance_buffer, m_exchange_acceptance_mutex)) {
        std::shared_ptr<types::ExchangeAcceptance> exchange_acceptance;
        {
            std::lock_guard lock(m_exchange_acceptance_mutex);

            if (m_exchange_acceptance_buffer.empty())
                continue;

            exchange_acceptance = m_exchange_acceptance_buffer[0];
            m_exchange_acceptance_buffer.pop_front();
        }
        m_outside->exchange_accepted(exchange_acceptance);
    }
}

void trade::reporter::AsyncReporter::do_order_rejected()
{
    while (IS_RUNNING(m_order_rejection_buffer, m_order_rejection_mutex)) {
        std::shared_ptr<types::OrderRejection> order_rejection;
        {
            std::lock_guard lock(m_order_rejection_mutex);

            if (m_order_rejection_buffer.empty())
                continue;

            order_rejection = m_order_rejection_buffer[0];
            m_order_rejection_buffer.pop_front();
        }
        m_outside->order_rejected(order_rejection);
    }
}

void trade::reporter::AsyncReporter::do_cancel_broker_accepted()
{
    while (IS_RUNNING(m_cancel_broker_acceptance_buffer, m_cancel_broker_acceptance_mutex)) {
        std::shared_ptr<types::CancelBrokerAcceptance> cancel_broker_acceptance;
        {
            std::lock_guard lock(m_cancel_broker_acceptance_mutex);

            if (m_cancel_broker_acceptance_buffer.empty())
                continue;

            cancel_broker_acceptance = m_cancel_broker_acceptance_buffer[0];
            m_cancel_broker_acceptance_buffer.pop_front();
        }
        m_outside->cancel_broker_accepted(cancel_broker_acceptance);
    }
}

void trade::reporter::AsyncReporter::do_cancel_exchange_accepted()
{
    while (IS_RUNNING(m_cancel_exchange_acceptance_buffer, m_cancel_exchange_acceptance_mutex)) {
        std::shared_ptr<types::CancelExchangeAcceptance> cancel_exchange_acceptance;
        {
            std::lock_guard lock(m_cancel_exchange_acceptance_mutex);

            if (m_cancel_exchange_acceptance_buffer.empty())
                continue;

            cancel_exchange_acceptance = m_cancel_exchange_acceptance_buffer[0];
            m_cancel_exchange_acceptance_buffer.pop_front();
        }
        m_outside->cancel_exchange_accepted(cancel_exchange_acceptance);
    }
}

void trade::reporter::AsyncReporter::do_cancel_success()
{
    while (IS_RUNNING(m_cancel_success_buffer, m_cancel_success_mutex)) {
        std::shared_ptr<types::CancelSuccess> cancel_success;
        {
            std::lock_guard lock(m_cancel_success_mutex);

            if (m_cancel_success_buffer.empty())
                continue;

            cancel_success = m_cancel_success_buffer[0];
            m_cancel_success_buffer.pop_front();
        }
        m_outside->cancel_success(cancel_success);
    }
}

void trade::reporter::AsyncReporter::do_cancel_order_rejected()
{
    while (IS_RUNNING(m_cancel_order_rejection_buffer, m_cancel_order_rejection_mutex)) {
        std::shared_ptr<types::CancelOrderRejection> cancel_order_rejection;
        {
            std::lock_guard lock(m_cancel_order_rejection_mutex);

            if (m_cancel_order_rejection_buffer.empty())
                continue;

            cancel_order_rejection = m_cancel_order_rejection_buffer[0];
            m_cancel_order_rejection_buffer.pop_front();
        }
        m_outside->cancel_order_rejected(cancel_order_rejection);
    }
}

void trade::reporter::AsyncReporter::do_trade_accepted()
{
    while (IS_RUNNING(m_trade_buffer, m_trade_mutex)) {
        std::shared_ptr<types::Trade> trade;
        {
            std::lock_guard lock(m_trade_mutex);

            if (m_trade_buffer.empty())
                continue;

            trade = m_trade_buffer[0];
            m_trade_buffer.pop_front();
        }
        m_outside->trade_accepted(trade);
    }
}

void trade::reporter::AsyncReporter::do_md_trade_generated()
{
    while (IS_RUNNING(m_md_trade_buffer, m_md_trade_mutex)) {
        std::shared_ptr<types::MdTrade> md_trade;
        {
            std::lock_guard lock(m_md_trade_mutex);

            if (m_md_trade_buffer.empty())
                continue;

            md_trade = m_md_trade_buffer[0];
            m_md_trade_buffer.pop_front();
        }
        m_outside->md_trade_generated(md_trade);
    }
}
