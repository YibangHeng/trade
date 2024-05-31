#include "libbooker/Rearranger.h"

trade::booker::Rearranger::Rearranger(const int64_t starting_order_id)
    : excepted(starting_order_id)
{
}

void trade::booker::Rearranger::push(const OrderWrapperPtr& order_wrapper)
{
    cached_orders.push(order_wrapper);
}

std::optional<trade::booker::OrderWrapperPtr> trade::booker::Rearranger::pop()
{
    if (cached_orders.empty()
        || excepted != cached_orders.top()->unique_id()) {
        return std::nullopt;
    }

    auto next_order_wrapper = cached_orders.top();
    cached_orders.pop();
    excepted++;

    return std::make_optional(std::move(next_order_wrapper));
}