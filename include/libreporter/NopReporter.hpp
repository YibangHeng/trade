#pragma once

#include "IReporter.hpp"

namespace trade::reporter
{

/// Do nothing but forward to the outside.
/// It can also be the base reporter for those who do not want to implement
/// everything.
class PUBLIC_API NopReporter: public IReporter
{
public:
    explicit NopReporter(const std::shared_ptr<IReporter>& outside = nullptr) : m_outside(outside) {}
    ~NopReporter() override = default;

    /// Order.
public:
    void broker_accepted(const std::shared_ptr<types::BrokerAcceptance> broker_acceptance) override
    {
        if (m_outside != nullptr) m_outside->broker_accepted(broker_acceptance);
    }
    void exchange_accepted(const std::shared_ptr<types::ExchangeAcceptance> exchange_acceptance) override
    {
        if (m_outside != nullptr) m_outside->exchange_accepted(exchange_acceptance);
    }
    void order_rejected(const std::shared_ptr<types::OrderRejection> order_rejection) override
    {
        if (m_outside != nullptr) m_outside->order_rejected(order_rejection);
    }

    /// Cancel.
public:
    void cancel_broker_accepted(const std::shared_ptr<types::CancelBrokerAcceptance> cancel_broker_acceptance) override
    {
        if (m_outside != nullptr) m_outside->cancel_broker_accepted(cancel_broker_acceptance);
    }
    void cancel_exchange_accepted(const std::shared_ptr<types::CancelExchangeAcceptance> cancel_exchange_acceptance) override
    {
        if (m_outside != nullptr) m_outside->cancel_exchange_accepted(cancel_exchange_acceptance);
    }
    void cancel_success(const std::shared_ptr<types::CancelSuccess> cancel_success) override
    {
        if (m_outside != nullptr) m_outside->cancel_success(cancel_success);
    }
    void cancel_order_rejected(const std::shared_ptr<types::CancelOrderRejection> cancel_order_rejection) override
    {
        if (m_outside != nullptr) m_outside->cancel_order_rejected(cancel_order_rejection);
    }

    /// Trade.
public:
    void trade_accepted(const std::shared_ptr<types::Trade> trade) override
    {
        if (m_outside != nullptr) m_outside->trade_accepted(trade);
    }

    /// Market data.
public:
    void exchange_order_tick_arrived(const std::shared_ptr<types::OrderTick> order_tick) override
    {
        if (m_outside != nullptr) m_outside->exchange_order_tick_arrived(order_tick);
    }
    void exchange_trade_tick_arrived(const std::shared_ptr<types::TradeTick> trade_tick) override
    {
        if (m_outside != nullptr) m_outside->exchange_trade_tick_arrived(trade_tick);
    }
    void exchange_l2_tick_arrived(const std::shared_ptr<types::ExchangeL2Snap> exchange_l2_snap) override
    {
        if (m_outside != nullptr) m_outside->exchange_l2_tick_arrived(exchange_l2_snap);
    }
    void l2_tick_generated(const std::shared_ptr<types::GeneratedL2Tick> generated_l2_tick) override
    {
        if (m_outside != nullptr) m_outside->l2_tick_generated(generated_l2_tick);
    }

private:
    std::shared_ptr<IReporter> m_outside;
};

} // namespace trade::reporter
