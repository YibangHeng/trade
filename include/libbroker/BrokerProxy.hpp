#pragma once

#include "AppBase.hpp"
#include "IBroker.h"
#include "libholder/IHolder.h"
#include "libreporter/IReporter.hpp"
#include "utilities/LoginSyncer.hpp"
#include "utilities/TimeHelper.hpp"
#include "utilities/ToJSON.hpp"

namespace trade::broker
{

template<typename TickerTaperT = int64_t, utilities::ConfigFileType ConfigFileType = utilities::ConfigFileType::INI>
class BrokerProxy
    : public IBroker,
      public utilities::LoginSyncer,
      public AppBase<TickerTaperT, ConfigFileType>
{
public:
    explicit BrokerProxy(
        const std::string& name,
        const std::shared_ptr<holder::IHolder>& holder,
        const std::shared_ptr<reporter::IReporter>& reporter,
        const std::string& config_path = "./etc/trade.ini"
    ) : AppBase<TickerTaperT, ConfigFileType>(name, config_path),
        m_holder(holder),
        m_reporter(reporter)
    {
    }

    ~BrokerProxy() override = default;

public:
    void login() noexcept override { start_login(); }
    void wait_login() noexcept(false) override { wait_login_reasult(); }

    void logout() noexcept override { start_logout(); }
    void wait_logout() noexcept(false) override { wait_logout_reasult(); }

public:
    int64_t new_order(std::shared_ptr<types::NewOrderReq> new_order_req) override
    {
        if (!new_order_req->has_request_id()) {
            new_order_req->set_request_id(AppBase<TickerTaperT, ConfigFileType>::snow_flaker());
        }

        if (!new_order_req->has_unique_id()) {
            new_order_req->set_unique_id(AppBase<TickerTaperT, ConfigFileType>::snow_flaker());
        }

        /// Pre-create order in holder.
        if (!pre_insert_order(new_order_req)) {
            logger->error("Failed to pre-create order due to holder error: {}", utilities::ToJSON()(*new_order_req));
            return INVALID_ID;
        }

        logger->info("New order pre-created: {}", utilities::ToJSON()(*new_order_req));
        return new_order_req->unique_id();
    }

    int64_t cancel_order(std::shared_ptr<types::NewCancelReq> new_cancel_req) override { return INVALID_ID; }
    int64_t cancel_all(std::shared_ptr<types::NewCancelAllReq> new_cancel_all_req) override { return INVALID_ID; }

private:
    [[nodiscard]] bool pre_insert_order(const std::shared_ptr<types::NewOrderReq>& new_order_req) const
    {
        const auto orders = std::make_shared<types::Orders>();

        types::Order order;

        order.set_unique_id(new_order_req->unique_id());
        order.clear_broker_id();   /// No broker_id yet.
        order.clear_exchange_id(); /// No exchange_id yet.
        order.set_symbol(new_order_req->symbol());
        order.set_side(new_order_req->side());
        order.set_position_side(new_order_req->position_side());
        order.set_price(new_order_req->price());
        order.set_quantity(new_order_req->quantity());
        order.set_allocated_creation_time(utilities::Now<google::protobuf::Timestamp*>()());
        order.set_allocated_update_time(utilities::Now<google::protobuf::Timestamp*>()());

        orders->add_orders()->CopyFrom(order);

        /// Return true if the order is inserted successfully.
        return m_holder->update_orders(orders) == 1;
    }

protected:
    std::shared_ptr<holder::IHolder> m_holder;
    std::shared_ptr<reporter::IReporter> m_reporter;

private:
    decltype(AppBase<TickerTaperT, ConfigFileType>::logger) logger = AppBase<TickerTaperT, ConfigFileType>::logger;
};

} // namespace trade::broker
