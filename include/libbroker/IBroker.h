#pragma once

#include "types.pb.h"
#include "visibility.h"

namespace trade::broker
{

class PUBLIC_API IBroker
{
public:
    explicit IBroker() = default;
    virtual ~IBroker() = default;

public:
    /// Initialize SDK and login.
    virtual void login() noexcept = 0;
    /// Wait until SDK and login is finished.
    virtual void wait_login() noexcept(false) = 0;

    /// Logout from SDK.
    virtual void logout() noexcept = 0;
    /// Wait until logout is finished.
    virtual void wait_logout() noexcept(false) = 0;

public:
    virtual int64_t new_order(types::NewOrderReq&& new_order_req)           = 0;
    virtual int64_t cancel_order(types::NewCancelReq&& new_cancel_req)      = 0;
    virtual int64_t cancel_all(types::NewCancelAllReq&& new_cancel_all_req) = 0;
};

} // namespace trade::broker
