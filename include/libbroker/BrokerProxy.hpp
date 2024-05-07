#pragma once

#include "AppBase.hpp"
#include "IBroker.h"
#include "libholder/IHolder.h"
#include "utilities/LoginSyncer.hpp"

namespace trade::broker
{

template<typename TickerTaperT = int64_t, utilities::ConfigFileType ConfigFileType = utilities::ConfigFileType::INI>
class BrokerProxy: public IBroker, public utilities::LoginSyncer, public AppBase<TickerTaperT, ConfigFileType>
{
public:
    explicit BrokerProxy(
        const std::string& name,
        const std::shared_ptr<holder::IHolder>& holder,
        const std::string& config_path = "./etc/trade.ini"
    ) : AppBase<TickerTaperT, ConfigFileType>(name, config_path),
        m_holder(holder)
    {
    }

    ~BrokerProxy() override = default;

public:
    void login() noexcept override { start_login(); }
    void wait_login() noexcept(false) override { wait_login_reasult(); }

    void logout() noexcept override { start_logout(); }
    void wait_logout() noexcept(false) override { wait_logout_reasult(); }

public:
    int64_t new_order(types::NewOrderReq&& new_order_req) override { return 0; }
    int64_t cancel_order(types::NewCancelReq&& new_cancel_req) override { return 0; }
    int64_t cancel_all(types::NewCancelAllReq&& new_cancel_all_req) override { return 0; }

protected:
    std::shared_ptr<holder::IHolder> m_holder;
};

} // namespace trade::broker
