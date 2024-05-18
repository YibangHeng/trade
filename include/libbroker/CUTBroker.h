#pragma once

#include <memory>

#include "BrokerProxy.hpp"
#include "CUTImpl/CUTTraderImpl.h"

namespace trade::broker
{

class PUBLIC_API CUTBroker final: public BrokerProxy<int>
{
public:
    explicit CUTBroker(
        const std::string& config_path,
        const std::shared_ptr<holder::IHolder>& holder,
        const std::shared_ptr<reporter::IReporter>& reporter
    );
    ~CUTBroker() override = default;

public:
    void start_login() noexcept override;
    void start_logout() noexcept override;

public:
    std::shared_ptr<types::NewOrderRsp> new_order(std::shared_ptr<types::NewOrderReq> new_order_req) override;
    std::shared_ptr<types::NewCancelRsp> cancel_order(std::shared_ptr<types::NewCancelReq> new_cancel_req) override;

public:
    void subscribe(const std::unordered_set<std::string>& symbols) override;
    void unsubscribe(const std::unordered_set<std::string>& symbols) override;

private:
    std::unique_ptr<CUTTraderImpl> m_trader_impl;
};

} // namespace trade::broker
