#pragma once

#include <memory>

#include "orms.pb.h"
#include "visibility.h"

namespace trade::holder
{

class PUBLIC_API IHolder
{
public:
    explicit IHolder() = default;
    virtual ~IHolder() = default;

public:
    virtual int64_t update_funds(std::shared_ptr<types::Funds> funds)                                  = 0;
    virtual std::shared_ptr<types::Funds> query_funds_by_account_id(const std::string& account_id)     = 0;

    virtual int64_t init_positions(std::shared_ptr<types::Positions> positions)                        = 0;

    virtual int64_t update_orders(std::shared_ptr<types::Orders> orders)                               = 0;
    virtual std::shared_ptr<types::Orders> query_orders_by_unique_id(int64_t unique_id)                = 0;
    virtual std::shared_ptr<types::Orders> query_orders_by_broker_id(const std::string& broker_id)     = 0;
    virtual std::shared_ptr<types::Orders> query_orders_by_exchange_id(const std::string& exchange_id) = 0;
};

} // namespace trade::holder
