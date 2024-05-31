#pragma once

#include "IReporter.hpp"

namespace trade::reporter
{

class PUBLIC_API NopReporter: public IReporter
{
public:
    NopReporter()           = default;
    ~NopReporter() override = default;

    /// Order.
public:
    void broker_accepted(std::shared_ptr<types::BrokerAcceptance> broker_acceptance) override {}
    void exchange_accepted(std::shared_ptr<types::ExchangeAcceptance> exchange_acceptance) override {}
    void order_rejected(std::shared_ptr<types::OrderRejection> order_rejection) override {}

    /// Cancel.
public:
    void cancel_broker_accepted(std::shared_ptr<types::CancelBrokerAcceptance> cancel_broker_acceptance) override {}
    void cancel_exchange_accepted(std::shared_ptr<types::CancelExchangeAcceptance> cancel_exchange_acceptance) override {}
    void cancel_success(std::shared_ptr<types::CancelSuccess> cancel_success) override {}
    void cancel_order_rejected(std::shared_ptr<types::CancelOrderRejection> cancel_order_rejection) override {}

    /// Trade.
public:
    void trade_accepted(std::shared_ptr<types::Trade> trade) override {}

    /// Market data.
public:
    void md_trade_generated(std::shared_ptr<types::MdTrade> md_trade) override {}
};

} // namespace trade::reporter