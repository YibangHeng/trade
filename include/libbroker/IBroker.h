#pragma once

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
};

} // namespace trade::broker
