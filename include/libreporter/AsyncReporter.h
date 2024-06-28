#pragma once

#include <boost/lockfree/spsc_queue.hpp>
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
    void exchange_order_tick_arrived(std::shared_ptr<types::OrderTick> order_tick) override;
    void exchange_trade_tick_arrived(std::shared_ptr<types::TradeTick> trade_tick) override;
    void exchange_l2_tick_arrived(std::shared_ptr<types::L2Tick> l2_tick) override;
    void l2_tick_generated(std::shared_ptr<types::L2Tick> l2_tick) override;

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
    void do_exchange_order_tick_arrived();
    void do_exchange_trade_tick_arrived();
    void do_exchange_l2_tick_arrived();
    void do_l2_tick_generated();

private:
    template<typename T>
    using BufferType = boost::lockfree::spsc_queue<std::shared_ptr<T>, boost::lockfree::capacity<1024 * 1024>>;

    /// Order.
    BufferType<types::BrokerAcceptance> m_broker_acceptance_buffer;
    std::thread m_broker_acceptance_thread;
    BufferType<types::ExchangeAcceptance> m_exchange_acceptance_buffer;
    std::thread m_exchange_acceptance_thread;
    BufferType<types::OrderRejection> m_order_rejection_buffer;
    std::thread m_order_rejection_thread;

    /// Cancel.
    BufferType<types::CancelBrokerAcceptance> m_cancel_broker_acceptance_buffer;
    std::thread m_cancel_broker_acceptance_thread;
    BufferType<types::CancelExchangeAcceptance> m_cancel_exchange_acceptance_buffer;
    std::thread m_cancel_exchange_acceptance_thread;
    BufferType<types::CancelSuccess> m_cancel_success_buffer;
    std::thread m_cancel_success_thread;
    BufferType<types::CancelOrderRejection> m_cancel_order_rejection_buffer;
    std::thread m_cancel_order_rejection_thread;

    /// Trade.
    BufferType<types::Trade> m_trade_buffer;
    std::thread m_trade_thread;

    /// Market data.
    BufferType<types::OrderTick> m_exchange_order_tick_buffer;
    std::thread m_exchange_order_tick_thread;
    BufferType<types::TradeTick> m_exchange_trade_tick_buffer;
    std::thread m_exchange_trade_tick_thread;
    BufferType<types::L2Tick> m_exchange_l2_tick_buffer;
    std::thread m_exchange_l2_tick_thread;
    BufferType<types::L2Tick> m_l2_tick_buffer;
    std::thread m_l2_tick_thread;

private:
    std::atomic<bool> m_is_running;

private:
    std::shared_ptr<IReporter> m_outside;
};

} // namespace trade::reporter
