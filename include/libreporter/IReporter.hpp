#pragma once

#include <memory>

#include "networks.pb.h"
#include "visibility.h"

namespace trade::reporter
{

constexpr static int64_t level_depth = 10;

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
    virtual void exchange_tick_arrived(std::shared_ptr<types::ExchangeTick> exchange_tick) = 0;
    virtual void exchange_l2_tick_arrived(std::shared_ptr<types::L2Tick> l2_tick)          = 0;
    virtual void l2_tick_generated(std::shared_ptr<types::L2Tick> l2_tick)                 = 0;
};

} // namespace trade::reporter
