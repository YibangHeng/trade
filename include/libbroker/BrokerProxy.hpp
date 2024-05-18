#pragma once

#include "AppBase.hpp"
#include "IBroker.h"
#include "libholder/IHolder.h"
#include "libreporter/IReporter.hpp"
#include "utilities/TimeHelper.hpp"
#include "utilities/ToJSON.hpp"

namespace trade::broker
{

template<typename TickerTaperT = int64_t, utilities::ConfigFileType ConfigFileType = utilities::ConfigFileType::INI>
class BrokerProxy
    : public IBroker,
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
    std::shared_ptr<types::NewOrderRsp> new_order(std::shared_ptr<types::NewOrderReq> new_order_req) override
    {
        auto new_order_rsp = std::make_shared<types::NewOrderRsp>();

        if (!new_order_req->has_request_id()) {
            new_order_req->set_request_id(AppBase<TickerTaperT, ConfigFileType>::snow_flaker());
        }

        if (!new_order_req->has_unique_id()) {
            new_order_req->set_unique_id(AppBase<TickerTaperT, ConfigFileType>::snow_flaker());
        }

        /// Pre-create order in holder.
        if (!pre_insert_order(new_order_req)) {
            AppBase<TickerTaperT, ConfigFileType>::logger->error("Failed to pre-create order due to holder error: {}", utilities::ToJSON()(*new_order_req));

            new_order_rsp->set_request_id(new_order_req->request_id());
            new_order_rsp->set_unique_id(INVALID_ID);
            new_order_rsp->set_allocated_creation_time(utilities::Now<google::protobuf::Timestamp*>()());
            new_order_rsp->set_rejection_code(types::RejectionCode::unknown);
            new_order_rsp->set_rejection_reason(fmt::format("Failed to pre-create order due to holder error: {}", utilities::ToJSON()(*new_order_req)));

            return INVALID_ID;
        }

        AppBase<TickerTaperT, ConfigFileType>::logger->info("New order pre-created: {}", utilities::ToJSON()(*new_order_req));

        new_order_rsp->set_request_id(new_order_req->request_id());
        new_order_rsp->set_unique_id(new_order_req->unique_id());
        new_order_rsp->set_allocated_creation_time(utilities::Now<google::protobuf::Timestamp*>()());

        return new_order_rsp;
    }

    std::shared_ptr<types::NewCancelRsp> cancel_order(std::shared_ptr<types::NewCancelReq> new_cancel_req) override
    {
        auto new_cancel_rsp = std::make_shared<types::NewCancelRsp>();

        if (!new_cancel_req->has_request_id()) {
            new_cancel_req->set_request_id(AppBase<TickerTaperT, ConfigFileType>::snow_flaker());
        }

        AppBase<TickerTaperT, ConfigFileType>::logger->info("New cancel pre-created: {}", utilities::ToJSON()(*new_cancel_req));

        new_cancel_rsp->set_request_id(new_cancel_req->request_id());
        new_cancel_rsp->set_original_unique_id(new_cancel_req->original_unique_id());
        new_cancel_rsp->set_allocated_creation_time(utilities::Now<google::protobuf::Timestamp*>()());

        return new_cancel_rsp;
    }

    std::shared_ptr<types::NewCancelAllRsp> cancel_all(std::shared_ptr<types::NewCancelAllReq> new_cancel_all_req) override
    {
        auto new_cancel_all_rsp = std::make_shared<types::NewCancelAllRsp>();

        if (!new_cancel_all_req->has_request_id()) {
            new_cancel_all_req->set_request_id(AppBase<TickerTaperT, ConfigFileType>::snow_flaker());
        }

        new_cancel_all_rsp->set_request_id(new_cancel_all_req->request_id());
        new_cancel_all_rsp->set_allocated_creation_time(utilities::Now<google::protobuf::Timestamp*>()());
        new_cancel_all_rsp->set_rejection_code(types::RejectionCode::unknown);
        new_cancel_all_rsp->set_rejection_reason("Not supported yet");

        return new_cancel_all_rsp;
    }

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
};

} // namespace trade::broker
