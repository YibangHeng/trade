#include <google/protobuf/util/time_util.h>

#include "libbroker/CTPBroker.h"

trade::broker::CTPBroker::CTPBroker(
    const std::string& config_path,
    const std::shared_ptr<holder::IHolder>& holder,
    const std::shared_ptr<reporter::IReporter>& reporter
) : BrokerProxy("CTPBroker", holder, reporter, config_path)
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

std::shared_ptr<trade::types::NewOrderRsp> trade::broker::CTPBroker::new_order(const std::shared_ptr<types::NewOrderReq> new_order_req)
{
    auto new_order_rsp = BrokerProxy::new_order(new_order_req);
    if (new_order_rsp->has_rejection_code())
        return new_order_rsp;

    m_trader_impl->new_order(new_order_req, new_order_rsp);

    return new_order_rsp;
}

std::shared_ptr<trade::types::NewCancelRsp> trade::broker::CTPBroker::cancel_order(const std::shared_ptr<types::NewCancelReq> new_cancel_req)
{
    auto new_cancel_rsp = BrokerProxy::cancel_order(new_cancel_req);
    if (new_cancel_rsp->has_rejection_code())
        return new_cancel_rsp;

    m_trader_impl->cancel_order(new_cancel_req, new_cancel_rsp);

    return new_cancel_rsp;
}

void trade::broker::CTPBroker::subscribe(const std::unordered_set<std::string>& symbols)
{
    m_md_impl->subscribe(symbols);
}

void trade::broker::CTPBroker::unsubscribe(const std::unordered_set<std::string>& symbols)
{
    m_md_impl->unsubscribe(symbols);
}
