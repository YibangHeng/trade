#pragma once

#include <memory>

#include "BrokerProxy.hpp"
#include "CTPImpl/CTPTraderImpl.h"

namespace trade::broker
{

class PUBLIC_API CTPBroker final: public BrokerProxy<int>
{
public:
    explicit CTPBroker(
        const std::string& config_path,
        const std::shared_ptr<holder::IHolder>& holder,
        const std::shared_ptr<reporter::IReporter>& reporter
    );
    ~CTPBroker() override = default;

public:
    void start_login() noexcept override;
    void start_logout() noexcept override;

public:
    int64_t new_order(std::shared_ptr<types::NewOrderReq> new_order_req) override;
    int64_t cancel_order(std::shared_ptr<types::NewCancelReq> new_cancel_req) override;

private:
    std::unique_ptr<CTPTraderImpl> m_trader_impl;
};

} // namespace trade::broker
