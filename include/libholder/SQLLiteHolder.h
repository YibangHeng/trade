#pragma once

#include <sqlite3.h>

#include "IHolder.h"

#include <AppBase.hpp>

namespace trade::holder
{

class SQLLiteHolder final: public IHolder, private AppBase<>
{
public:
    explicit SQLLiteHolder(const std::string& db_path = ":memory:");
    ~SQLLiteHolder() override;

public:
    int64_t init_funds(types::Funds&& funds) override;
    int64_t init_positions(types::Positions&& positions) override;
    int64_t init_orders(types::Orders&& orders) override;

private:
    void start_transaction() const;
    void commit_or_rollback(decltype(SQLITE_OK) code) const;

private:
    void init_funds_table();
    void init_positions_table();

private:
    sqlite3* m_db;
    /// Funds table.
    const std::string m_fund_table_name;
    sqlite3_stmt* m_fund_insert_stmt;
    /// Positions table.
    const std::string m_position_table_name;
    sqlite3_stmt* m_position_insert_stmt;
    /// Orders table.
    const std::string m_order_table_name;
    sqlite3_stmt* m_order_insert_stmt;
};

} // namespace trade::holder