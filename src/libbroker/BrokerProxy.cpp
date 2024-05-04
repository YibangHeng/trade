#include "libbroker/BrokerProxy.h"

trade::broker::BrokerProxy::BrokerProxy(
    const std::string& name,
    const std::string& config_path
) : AppBase(name, config_path)
{
}

void trade::broker::BrokerProxy::login() noexcept
{
    start_login();
}

void trade::broker::BrokerProxy::wait_login() noexcept(false)
{
    wait_login_reasult();
}

void trade::broker::BrokerProxy::logout() noexcept
{
    start_logout();
}

void trade::broker::BrokerProxy::wait_logout() noexcept(false)
{
    wait_logout_reasult();
}

int64_t trade::broker::BrokerProxy::new_order(types::NewOrderReq&& new_order_req)
{
    return 0;
}

int64_t trade::broker::BrokerProxy::cancel_order(types::NewCancelReq&& new_cancel_req)
{
    return 0;
}

int64_t trade::broker::BrokerProxy::cancel_all(types::NewCancelAllReq&& new_cancel_all_req)
{
    return 0;
}