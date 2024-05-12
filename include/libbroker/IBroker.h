#pragma once

#include <memory>
#include <unordered_set>

#include "networks.pb.h"
#include "utilities/LoginSyncer.hpp"

namespace trade::broker
{

class PUBLIC_API IBroker
    : public utilities::LoginSyncer /// A broker also provides a login syncer.
{
public:
    explicit IBroker()  = default;
    ~IBroker() override = default;

public:
    virtual int64_t new_order(std::shared_ptr<types::NewOrderReq> new_order_req)           = 0;
    virtual int64_t cancel_order(std::shared_ptr<types::NewCancelReq> new_cancel_req)      = 0;
    virtual int64_t cancel_all(std::shared_ptr<types::NewCancelAllReq> new_cancel_all_req) = 0;

public:
    virtual void subscribe(const std::unordered_set<std::string>& symbols)   = 0;
    virtual void unsubscribe(const std::unordered_set<std::string>& symbols) = 0;
};

} // namespace trade::broker
