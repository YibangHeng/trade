#pragma once

#include "IBroker.h"
#include "utilities/LoginSyncer.hpp"

#include <AppBase.hpp>

namespace trade::broker
{

class BrokerProxy: public IBroker, public utilities::LoginSyncer, public AppBase
{
public:
    explicit BrokerProxy(
        const std::string& name,
        const std::string& config_path = "./etc/trade.ini"
    );
    ~BrokerProxy() override = default;

public:
    void login() noexcept override;
    void wait_login() noexcept(false) override;

    void logout() noexcept override;
    void wait_logout() noexcept(false) override;

public:
    int64_t new_order(types::NewOrderReq&& new_order_req) override;
    int64_t cancel_order(types::NewCancelReq&& new_cancel_req) override;
    int64_t cancel_all(types::NewCancelAllReq&& new_cancel_all_req) override;
};

} // namespace trade::broker
