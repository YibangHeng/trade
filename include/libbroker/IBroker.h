#pragma once

#include <memory>
#include <unordered_set>

#include "networks.pb.h"
#include "utilities/LoginSyncer.hpp"

namespace trade::broker
{

/// IBroker defines the interface of a broker.
class TD_PUBLIC_API IBroker
    : public utilities::LoginSyncer /// A broker also provides a login syncer.
{
public:
    explicit IBroker()  = default;
    ~IBroker() override = default;

public:
    virtual std::shared_ptr<types::NewOrderRsp> new_order(std::shared_ptr<types::NewOrderReq> new_order_req)               = 0;
    virtual std::shared_ptr<types::NewCancelRsp> cancel_order(std::shared_ptr<types::NewCancelReq> new_cancel_req)         = 0;
    virtual std::shared_ptr<types::NewCancelAllRsp> cancel_all(std::shared_ptr<types::NewCancelAllReq> new_cancel_all_req) = 0;

public:
    virtual void subscribe(const std::unordered_set<std::string>& symbols)   = 0;
    virtual void unsubscribe(const std::unordered_set<std::string>& symbols) = 0;
};

} // namespace trade::broker
