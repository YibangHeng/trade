#pragma once

#include <utility>

#include "AppBase.hpp"
#include "IReporter.hpp"
#include "NopReporter.hpp"

namespace trade::reporter
{

class PUBLIC_API LogReporter final: public IReporter
{
public:
    explicit LogReporter(std::shared_ptr<IReporter> outside = std::make_shared<NopReporter>())
        : trade_logger(spdlog::default_logger()->clone("TRADE")),
          md_logger(spdlog::default_logger()->clone("MD")),
          m_outside(std::move(outside))
    {
    }
    ~LogReporter() override = default;

public:
    /// Order.
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
    std::shared_ptr<spdlog::logger> trade_logger;
    std::shared_ptr<spdlog::logger> md_logger;

private:
    std::shared_ptr<IReporter> m_outside;
};

} // namespace trade::reporter