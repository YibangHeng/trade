#include <filesystem>
#include <fmt/format.h>
#include <google/protobuf/util/json_util.h>

#include "libholder/SQLLiteHolder.h"

trade::holder::SQLLiteHolder::SQLLiteHolder(const std::string& db_path)
    : AppBase("SQLLiteHolder"),
      m_db(nullptr),
      m_fund_table_name("funds"),
      m_fund_insert_stmt(nullptr),
      m_position_table_name("positions"),
      m_position_insert_stmt(nullptr),
      m_order_table_name("orders"),
      m_order_insert_stmt(nullptr)
{
    /// Create parent folder if not exist.
    const std::filesystem::path path = db_path;
    create_directories(path.parent_path());

    /// Open database.
    sqlite3_open(db_path.c_str(), &m_db);
    if (m_db == nullptr) {
        throw std::runtime_error(fmt::format("Failed to open sqlite3 database: {}", std::string(sqlite3_errmsg(m_db))));
    }
    sqlite3_exec(m_db, "PRAGMA foreign_keys = ON;", nullptr, nullptr, nullptr);
}

trade::holder::SQLLiteHolder::~SQLLiteHolder()
{
    sqlite3_close(m_db);
}

int64_t trade::holder::SQLLiteHolder::init_funds(types::Funds&& funds)
{
    /// Initialize funds at first time.
    if (m_fund_insert_stmt == nullptr) {
        init_funds_table();
    }

    start_transaction();

    for (const auto& fund : funds.funds()) {
        sqlite3_reset(m_fund_insert_stmt);

        sqlite3_bind_text(m_fund_insert_stmt, 1, fund.account_id().c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_double(m_fund_insert_stmt, 2, fund.available_fund());
        sqlite3_bind_double(m_fund_insert_stmt, 3, fund.withdrawn_fund());
        sqlite3_bind_double(m_fund_insert_stmt, 4, fund.frozen_fund());
        sqlite3_bind_double(m_fund_insert_stmt, 5, fund.frozen_margin());
        sqlite3_bind_double(m_fund_insert_stmt, 6, fund.frozen_commission());

        const auto code = sqlite3_step(m_fund_insert_stmt);
        if (code != SQLITE_DONE) {
            logger->error("Failed to insert fund {}: {}", fund.account_id(), std::string(sqlite3_errmsg(m_db)));
        }
    }

    commit_or_rollback(SQLITE_OK);

    return funds.funds_size();
}

int64_t trade::holder::SQLLiteHolder::init_positions(types::Positions&& positions)
{
    return 0;
}

int64_t trade::holder::SQLLiteHolder::init_orders(types::Orders&& orders)
{
    return 0;
}

void trade::holder::SQLLiteHolder::start_transaction() const
{
    sqlite3_exec(m_db, "BEGIN TRANSACTION;", nullptr, nullptr, nullptr);
}

void trade::holder::SQLLiteHolder::commit_or_rollback(const decltype(SQLITE_OK) code) const
{
    if (code == SQLITE_OK) {
        sqlite3_exec(m_db, "COMMIT;", nullptr, nullptr, nullptr);
    }
    else {
        logger->error("Failed to execute SQL: {}", std::string(sqlite3_errmsg(m_db)));
        sqlite3_exec(m_db, "ROLLBACK;", nullptr, nullptr, nullptr);
    }
}

void trade::holder::SQLLiteHolder::init_funds_table()
{
    /// Create table for funds.
    const std::string& create_funds_table_sql = fmt::format(
        "CREATE TABLE IF NOT EXISTS {} ("
        "account_id        TEXT NOT NULL PRIMARY KEY,"
        "available_fund    REAL,"
        "withdrawn_fund    REAL,"
        "frozen_fund       REAL,"
        "frozen_margin     REAL,"
        "frozen_commission REAL"
        ");",
        m_fund_table_name
    );

    auto code = sqlite3_exec(m_db, create_funds_table_sql.c_str(), nullptr, nullptr, nullptr);

    if (code != SQLITE_OK) {
        throw std::runtime_error(fmt::format("Failed to create table {}: {}", m_fund_table_name, std::string(sqlite3_errmsg(m_db))));
    }

    /// Prepare insert statement.
    const std::string& fund_insert_sql = fmt::format(
        "INSERT OR REPLACE INTO {} VALUES(?, ?, ?, ?, ?, ?);",
        m_fund_table_name
    );

    code = sqlite3_prepare_v2(m_db, fund_insert_sql.c_str(), -1, &m_fund_insert_stmt, nullptr);

    if (code != SQLITE_OK) {
        throw std::runtime_error(fmt::format("Failed to prepare SQL: {}", std::string(sqlite3_errmsg(m_db))));
    }
}

void trade::holder::SQLLiteHolder::init_positions_table()
{
    /// Create table for positions.
    const std::string& create_positions_table_sql = fmt::format(
        "CREATE TABLE IF NOT EXISTS {} ("
        "symbol                         TEXT NOT NULL PRIMARY KEY,"
        "yesterday_position             INTEGER,"
        "today_position                 INTEGER,"
        "open_volume                    INTEGER,"
        "close_volume                   INTEGER,"
        "position_cost                  REAL,"
        "pre_margin                     REAL,"
        "used_margin                    REAL,"
        "frozen_margin                  REAL,"
        "open_cost                      REAL,"
        "update_time                    DATETIME"
        ");",
        m_position_table_name
    );

    auto code = sqlite3_exec(m_db, create_positions_table_sql.c_str(), nullptr, nullptr, nullptr);

    if (code != SQLITE_OK) {
        throw std::runtime_error(fmt::format("Failed to create table {}: {}", m_position_table_name, std::string(sqlite3_errmsg(m_db))));
    }

    /// Prepare insert statement.
    const std::string& position_insert_sql = fmt::format(
        "INSERT OR REPLACE INTO {} VALUES(?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);",
        m_position_table_name
    );

    code = sqlite3_prepare_v2(m_db, position_insert_sql.c_str(), -1, &m_position_insert_stmt, nullptr);

    if (code != SQLITE_OK) {
        throw std::runtime_error(fmt::format("Failed to prepare SQL: {}", std::string(sqlite3_errmsg(m_db))));
    }
}