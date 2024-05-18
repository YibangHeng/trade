#include <google/protobuf/util/time_util.h>

#include "libbroker/CUTBroker.h"

trade::broker::CUTBroker::CUTBroker(
    const std::string& config_path,
    const std::shared_ptr<holder::IHolder>& holder,
    const std::shared_ptr<reporter::IReporter>& reporter
) : BrokerProxy("CUTBroker", holder, reporter, config_path),
    m_trader_impl(nullptr)
{
}

void trade::broker::CUTBroker::start_login() noexcept
{
    BrokerProxy::start_login();

    m_trader_impl = std::make_unique<CUTTraderImpl>(config, m_holder, m_reporter, this);
}

void trade::broker::CUTBroker::start_logout() noexcept
{
    BrokerProxy::start_logout();

    m_trader_impl.reset();
}

std::shared_ptr<trade::types::NewOrderRsp> trade::broker::CUTBroker::new_order(const std::shared_ptr<types::NewOrderReq> new_order_req)
{
    auto new_order_rsp = BrokerProxy::new_order(new_order_req);
    if (new_order_rsp->has_rejection_code())
        return new_order_rsp;

    m_trader_impl->new_order(new_order_req, new_order_rsp);

    return new_order_rsp;
}

std::shared_ptr<trade::types::NewCancelRsp> trade::broker::CUTBroker::cancel_order(const std::shared_ptr<types::NewCancelReq> new_cancel_req)
{
    auto new_cancel_rsp = BrokerProxy::cancel_order(new_cancel_req);
    if (new_cancel_rsp->has_rejection_code())
        return new_cancel_rsp;

    m_trader_impl->cancel_order(new_cancel_req, new_cancel_rsp);

    return new_cancel_rsp;
}

void trade::broker::CUTBroker::subscribe(const std::unordered_set<std::string>& symbols)
{
    logger->warn("CUTBroker::subscribe is not implemented yet");
}

void trade::broker::CUTBroker::unsubscribe(const std::unordered_set<std::string>& symbols)
{
    logger->warn("CUTBroker::unsubscribe is not implemented yet");
}
