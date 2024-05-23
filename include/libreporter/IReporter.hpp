#pragma once

#include <memory>

#include "networks.pb.h"
#include "visibility.h"

namespace trade::reporter
{

class PUBLIC_API IReporter
{
public:
    explicit IReporter() = default;
    virtual ~IReporter() = default;

    /// Order.
public:
    virtual void broker_accepted(std::shared_ptr<types::BrokerAcceptance> broker_acceptance)       = 0;
    virtual void exchange_accepted(std::shared_ptr<types::ExchangeAcceptance> exchange_acceptance) = 0;
    virtual void order_rejected(std::shared_ptr<types::OrderRejection> order_rejection)            = 0;

    /// Cancel.
public:
    virtual void cancel_broker_accepted(std::shared_ptr<types::CancelBrokerAcceptance> cancel_broker_acceptance)       = 0;
    virtual void cancel_exchange_accepted(std::shared_ptr<types::CancelExchangeAcceptance> cancel_exchange_acceptance) = 0;
    virtual void cancel_success(std::shared_ptr<types::CancelSuccess> cancel_success)                                  = 0;
    virtual void cancel_order_rejected(std::shared_ptr<types::CancelOrderRejection> cancel_order_rejection)            = 0;

    /// Trade.
public:
    virtual void trade_accepted(std::shared_ptr<types::Trade> trade) = 0;

    /// Market data.
public:
    virtual void md_trade_generated(std::shared_ptr<types::MdTrade> md_trade) = 0;
};

} // namespace trade::reporter
