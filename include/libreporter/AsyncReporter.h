#pragma once

#include <boost/circular_buffer.hpp>
#include <thread>

#include "IReporter.hpp"
#include "NopReporter.hpp"

namespace trade::reporter
{

class PUBLIC_API AsyncReporter final: public IReporter
{
public:
    explicit AsyncReporter(std::shared_ptr<IReporter> outside = std::make_shared<NopReporter>());
    ~AsyncReporter() override;

    /// BUG: Buffer may overflow and cause the task to be lost.

    /// Order.
public:
    void broker_accepted(std::shared_ptr<types::BrokerAcceptance> broker_acceptance) override;
    void exchange_accepted(std::shared_ptr<types::ExchangeAcceptance> exchange_acceptance) override;
    void order_rejected(std::shared_ptr<types::OrderRejection> order_rejection) override;

    /// Cancel.
public:
    void cancel_broker_accepted(std::shared_ptr<types::CancelBrokerAcceptance> cancel_broker_acceptance) override;
    void cancel_exchange_accepted(std::shared_ptr<types::CancelExchangeAcceptance> cancel_exchange_acceptance) override;
    void cancel_success(std::shared_ptr<types::CancelSuccess> cancel_success) override;
    void cancel_order_rejected(std::shared_ptr<types::CancelOrderRejection> cancel_order_rejection) override;

    /// Trade.
public:
    void trade_accepted(std::shared_ptr<types::Trade> trade) override;

    /// Market data.
public:
    void md_trade_generated(std::shared_ptr<types::MdTrade> md_trade) override;

private:
    /// Order.
public:
    void do_broker_accepted();
    void do_exchange_accepted();
    void do_order_rejected();

    /// Cancel.
public:
    void do_cancel_broker_accepted();
    void do_cancel_exchange_accepted();
    void do_cancel_success();
    void do_cancel_order_rejected();

    /// Trade.
public:
    void do_trade_accepted();

    /// Market data.
public:
    void do_md_trade_generated();

private:
    /// Order.
    boost::circular_buffer<std::shared_ptr<types::BrokerAcceptance>> m_broker_acceptance_buffer;
    std::mutex m_broker_acceptance_mutex;
    std::thread m_broker_acceptance_thread;
    boost::circular_buffer<std::shared_ptr<types::ExchangeAcceptance>> m_exchange_acceptance_buffer;
    std::mutex m_exchange_acceptance_mutex;
    std::thread m_exchange_acceptance_thread;
    boost::circular_buffer<std::shared_ptr<types::OrderRejection>> m_order_rejection_buffer;
    std::mutex m_order_rejection_mutex;
    std::thread m_order_rejection_thread;

    /// Cancel.
    boost::circular_buffer<std::shared_ptr<types::CancelBrokerAcceptance>> m_cancel_broker_acceptance_buffer;
    std::mutex m_cancel_broker_acceptance_mutex;
    std::thread m_cancel_broker_acceptance_thread;
    boost::circular_buffer<std::shared_ptr<types::CancelExchangeAcceptance>> m_cancel_exchange_acceptance_buffer;
    std::mutex m_cancel_exchange_acceptance_mutex;
    std::thread m_cancel_exchange_acceptance_thread;
    boost::circular_buffer<std::shared_ptr<types::CancelSuccess>> m_cancel_success_buffer;
    std::mutex m_cancel_success_mutex;
    std::thread m_cancel_success_thread;
    boost::circular_buffer<std::shared_ptr<types::CancelOrderRejection>> m_cancel_order_rejection_buffer;
    std::mutex m_cancel_order_rejection_mutex;
    std::thread m_cancel_order_rejection_thread;

    /// Trade.
    boost::circular_buffer<std::shared_ptr<types::Trade>> m_trade_buffer;
    std::mutex m_trade_mutex;
    std::thread m_trade_thread;

    /// Market data.
    boost::circular_buffer<std::shared_ptr<types::MdTrade>> m_md_trade_buffer;
    std::mutex m_md_trade_mutex;
    std::thread m_md_trade_thread;

private:
    std::atomic<bool> is_running;

private:
    static constexpr size_t buffer_size = 1024 * 1024 * 1024; /// Make a big buffer.

private:
    std::shared_ptr<IReporter> m_outside;
};

} // namespace trade::reporter