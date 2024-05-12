#include <google/protobuf/util/time_util.h>
#include <regex>

#include "libbroker/CTPBroker.h"

trade::broker::CTPBroker::CTPBroker(
    const std::string& config_path,
    const std::shared_ptr<holder::IHolder>& holder,
    const std::shared_ptr<reporter::IReporter>& reporter
) : BrokerProxy("CTPBroker", holder, reporter, config_path),
    m_trader_impl(nullptr)
{
}

void trade::broker::CTPBroker::start_login() noexcept
{
    BrokerProxy::start_login();

    m_md_impl     = std::make_unique<CTPMdImpl>(config, m_holder, m_reporter);
    m_trader_impl = std::make_unique<CTPTraderImpl>(config, m_holder, m_reporter, this);
}

void trade::broker::CTPBroker::start_logout() noexcept
{
    BrokerProxy::start_logout();

    m_md_impl.reset();
    m_trader_impl.reset();
}

int64_t trade::broker::CTPBroker::new_order(const std::shared_ptr<types::NewOrderReq> new_order_req)
{
    if (BrokerProxy::new_order(new_order_req) == INVALID_ID)
        return INVALID_ID;

    return m_trader_impl->new_order(new_order_req);
}

int64_t trade::broker::CTPBroker::cancel_order(const std::shared_ptr<types::NewCancelReq> new_cancel_req)
{
    if (BrokerProxy::cancel_order(new_cancel_req) == INVALID_ID)
        return INVALID_ID;

    return m_trader_impl->cancel_order(new_cancel_req);
}
