#pragma once

#include <spdlog/spdlog.h>
#include <utility>

#include "IReporter.hpp"
#include "NopReporter.hpp"

namespace trade::reporter
{

class TD_PUBLIC_API LogReporter final: public IReporter
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
    void exchange_order_tick_arrived(std::shared_ptr<types::OrderTick> order_tick) override;
    void exchange_trade_tick_arrived(std::shared_ptr<types::TradeTick> trade_tick) override;
    void exchange_l2_snap_arrived(std::shared_ptr<types::ExchangeL2Snap> exchange_l2_snap) override;
    void l2_tick_generated(std::shared_ptr<types::GeneratedL2Tick> generated_l2_tick) override;
    void ranged_tick_generated(std::shared_ptr<types::RangedTick> ranged_tick) override;

private:
    std::shared_ptr<spdlog::logger> trade_logger;
    std::shared_ptr<spdlog::logger> md_logger;

private:
    std::shared_ptr<IReporter> m_outside;
};

} // namespace trade::reporter