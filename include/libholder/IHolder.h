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
    virtual int64_t init_funds(types::Funds&& funds)             = 0;
    virtual int64_t init_positions(types::Positions&& positions) = 0;
    virtual int64_t init_orders(types::Orders&& orders)          = 0;
};

} // namespace trade::holder
