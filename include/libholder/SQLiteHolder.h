#pragma once

#include <sqlite3.h>

#include "AppBase.hpp"
#include "IHolder.h"

#define SQLITE_AUTO_LENGTH (-1)

namespace trade::holder
{

struct SQLite3PtrDeleter {
    void operator()(sqlite3* db) const
    {
        sqlite3_close(db);
    }
};

using SQLite3Ptr = std::unique_ptr<sqlite3, SQLite3PtrDeleter>;

struct SQLite3StmtPtrDeleter {
    void operator()(sqlite3_stmt* stmt) const
    {
        sqlite3_finalize(stmt);
    }
};

using SQLite3StmtPtr = std::unique_ptr<sqlite3_stmt, SQLite3StmtPtrDeleter>;

class TD_PUBLIC_API SQLiteHolder final: public IHolder, private AppBase<>
{
public:
    explicit SQLiteHolder(const std::string& db_path = ":memory:");
    ~SQLiteHolder() override = default;

public:
    int64_t update_symbols(std::shared_ptr<types::Symbols> symbols) override;
    std::shared_ptr<types::Symbols> query_symbols_by_symbol(const std::string& symbol) override;
    std::shared_ptr<types::Symbols> query_symbols_by_exchange(types::ExchangeType exchange) override;

    int64_t update_funds(std::shared_ptr<types::Funds> funds) override;
    std::shared_ptr<types::Funds> query_funds_by_account_id(const std::string& account_id) override;

    int64_t update_positions(std::shared_ptr<types::Positions> positions) override;
    std::shared_ptr<types::Positions> query_positions_by_symbol(const std::string& symbol) override;

    int64_t update_orders(std::shared_ptr<types::Orders> orders) override;
    std::shared_ptr<types::Orders> query_orders_by_unique_id(int64_t unique_id) override;
    std::shared_ptr<types::Orders> query_orders_by_broker_id(const std::string& broker_id) override;
    std::shared_ptr<types::Orders> query_orders_by_exchange_id(const std::string& exchange_id) override;

private:
    void start_transaction() const;
    void commit_or_rollback(decltype(SQLITE_OK) code) const;

private:
    void init_symbol_table();
    void init_query_symbol_stmts();
    std::shared_ptr<types::Symbols> query_symbols_by(const std::string& by, const std::string& id) const;

    void init_fund_table();
    void init_query_fund_stmts();

    void init_position_table();
    void init_query_position_stmts();

    void init_order_table();
    void init_query_order_stmts();
    std::shared_ptr<types::Orders> query_orders_by(const std::string& by, const std::string& id) const;

    void prepare_database();

private:
    [[nodiscard]] static std::string to_exchange(types::ExchangeType exchange);
    [[nodiscard]] static types::ExchangeType to_exchange(const std::string& exchange);
    [[nodiscard]] static std::string to_side(types::SideType side);
    [[nodiscard]] static types::SideType to_side(const std::string& side);
    [[nodiscard]] static std::string to_position_side(types::PositionSideType position_side);
    [[nodiscard]] static types::PositionSideType to_position_side(const std::string& position_side);

private:
    SQLite3Ptr m_db;
    decltype(SQLITE_OK) m_exec_code;
    /// Symbols table.
    const std::string m_symbol_table_name;
    SQLite3StmtPtr m_insert_symbols;
    std::unordered_map<std::string, SQLite3StmtPtr> m_query_symbols_by;
    /// Funds table.
    const std::string m_fund_table_name;
    SQLite3StmtPtr m_insert_funds;
    SQLite3StmtPtr m_query_funds_by_account_id;
    /// Positions table.
    const std::string m_position_table_name;
    SQLite3StmtPtr m_insert_positions;
    SQLite3StmtPtr m_query_positions_by_symbol;
    /// Orders table.
    const std::string m_order_table_name;
    SQLite3StmtPtr m_insert_orders;
    std::unordered_map<std::string, SQLite3StmtPtr> m_query_orders_by;
};

} // namespace trade::holder
