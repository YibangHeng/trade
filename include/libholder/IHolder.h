#pragma once

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
    virtual int64_t init_funds(std::shared_ptr<types::Funds> funds)             = 0;
    virtual int64_t init_positions(std::shared_ptr<types::Positions> positions) = 0;
    virtual int64_t init_orders(std::shared_ptr<types::Orders> orders)          = 0;
};

} // namespace trade::holder
