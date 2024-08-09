#pragma once

#include <memory>

#include "orms.pb.h"
#include "visibility.h"

namespace trade::holder
{

class TD_PUBLIC_API IHolder
{
public:
    explicit IHolder() = default;
    virtual ~IHolder() = default;

public:
    virtual int64_t update_symbols(std::shared_ptr<types::Symbols> symbols)                            = 0;
    virtual std::shared_ptr<types::Symbols> query_symbols_by_symbol(const std::string& symbol)         = 0;
    virtual std::shared_ptr<types::Symbols> query_symbols_by_exchange(types::ExchangeType exchange)    = 0;

    virtual int64_t update_funds(std::shared_ptr<types::Funds> funds)                                  = 0;
    virtual std::shared_ptr<types::Funds> query_funds_by_account_id(const std::string& account_id)     = 0;

    virtual int64_t update_positions(std::shared_ptr<types::Positions> positions)                      = 0;
    virtual std::shared_ptr<types::Positions> query_positions_by_symbol(const std::string& symbol)     = 0;

    virtual int64_t update_orders(std::shared_ptr<types::Orders> orders)                               = 0;
    virtual std::shared_ptr<types::Orders> query_orders_by_unique_id(int64_t unique_id)                = 0;
    virtual std::shared_ptr<types::Orders> query_orders_by_broker_id(const std::string& broker_id)     = 0;
    virtual std::shared_ptr<types::Orders> query_orders_by_exchange_id(const std::string& exchange_id) = 0;
};

} // namespace trade::holder
