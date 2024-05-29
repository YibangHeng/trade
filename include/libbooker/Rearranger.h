#pragma once

#include <memory>
#include <optional>
#include <queue>

#include "OrderWrapper.h"

namespace trade::booker
{

class OrderWrapperComparer
{
public:
    bool operator()(const OrderWrapperPtr& bigger, const OrderWrapperPtr& smaller) const
    {
        return bigger->unique_id() > smaller->unique_id();
    }
};

class Rearranger
{
public:
    explicit Rearranger(int64_t starting_order_id = 0);
    ~Rearranger() = default;

public:
    void push(const OrderWrapperPtr& order_wrapper);
    std::optional<OrderWrapperPtr> pop();

private:
    std::priority_queue<OrderWrapperPtr, std::deque<OrderWrapperPtr>, OrderWrapperComparer> cached_orders;
    int64_t excepted;
};

} // namespace trade::booker
