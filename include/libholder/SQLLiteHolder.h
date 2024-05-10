#pragma once

#include <sqlite3.h>

#include "AppBase.hpp"
#include "IHolder.h"

#define SQLITE_AUTO_LENGTH (-1)

namespace trade::holder
{

class SQLLiteHolder final: public IHolder, private AppBase<>
{
public:
    explicit SQLLiteHolder(const std::string& db_path = ":memory:");
    ~SQLLiteHolder() override;

public:
    int64_t init_funds(std::shared_ptr<types::Funds> funds) override;
    int64_t init_positions(std::shared_ptr<types::Positions> positions) override;

    int64_t update_orders(std::shared_ptr<types::Orders> orders) override;
    std::shared_ptr<types::Orders> query_orders_by_unique_id(int64_t unique_id) override;
    std::shared_ptr<types::Orders> query_orders_by_broker_id(const std::string& broker_id) override;
    std::shared_ptr<types::Orders> query_orders_by_exchange_id(const std::string& exchange_id) override;

private:
    void start_transaction() const;
    void commit_or_rollback(decltype(SQLITE_OK) code) const;

private:
    void init_funds_table();
    void init_positions_table();

    void init_orders_table();
    void init_query_orders_stmts();
    std::shared_ptr<types::Orders> query_orders_by(const std::string& by, const std::string& id) const;

    void init_database();

private:
    [[nodiscard]] static std::string to_exchange(types::ExchangeType exchange);
    [[nodiscard]] static types::ExchangeType to_exchange(const std::string& exchange);
    [[nodiscard]] static std::string to_side(types::SideType side);
    [[nodiscard]] static types::SideType to_side(const std::string& side);
    [[nodiscard]] static std::string to_position_side(types::PositionSideType position_side);
    [[nodiscard]] static types::PositionSideType to_position_side(const std::string& position_side);

private:
    sqlite3* m_db;
    decltype(SQLITE_OK) m_exec_code;
    /// Funds table.
    const std::string m_fund_table_name;
    sqlite3_stmt* m_fund_insert_stmt;
    /// Positions table.
    const std::string m_position_table_name;
    sqlite3_stmt* m_position_insert_stmt;
    /// Orders table.
    const std::string m_order_table_name;
    sqlite3_stmt* m_order_insert_stmt;
    sqlite3_stmt* m_query_orders_by_unique_id_stmt;
    sqlite3_stmt* m_query_orders_by_broker_id_stmt;
    sqlite3_stmt* m_query_orders_by_exchange_id_stmt;
};

} // namespace trade::holder
