#include <filesystem>
#include <fmt/format.h>
#include <google/protobuf/util/json_util.h>

#include "libholder/SQLiteHolder.h"
#include "utilities/TimeHelper.hpp"
#include "utilities/ToJSON.hpp"

trade::holder::SQLiteHolder::SQLiteHolder(const std::string& db_path)
    : AppBase("SQLiteHolder"),
      m_symbol_table_name("symbols"),
      m_fund_table_name("funds"),
      m_position_table_name("positions"),
      m_order_table_name("orders")
{
    /// Create parent folder if not exist.
    if (db_path != ":memory:") {
        const std::filesystem::path path = db_path;
        if (!exists(path.parent_path())) {
            create_directories(path.parent_path());
        }
    }

    /// Open database.
    sqlite3* raw_db;
    sqlite3_open(db_path.c_str(), &raw_db);

    m_db.reset(raw_db); /// Hand over ownership to m_db.

    if (m_db == nullptr) {
        throw std::runtime_error(fmt::format("Failed to open sqlite3 database: {}", std::string(sqlite3_errmsg(m_db.get()))));
    }

    m_exec_code = sqlite3_exec(m_db.get(), "PRAGMA foreign_keys = ON;", nullptr, nullptr, nullptr);

    if (m_exec_code != SQLITE_OK) {
        throw std::runtime_error(fmt::format("Failed to enable foreign keys: {}", std::string(sqlite3_errmsg(m_db.get()))));
    }

    prepare_database();
}

int64_t trade::holder::SQLiteHolder::update_symbols(const std::shared_ptr<types::Symbols> symbols)
{
    start_transaction();

    for (const auto& symbol : symbols->symbols()) {
        sqlite3_reset(m_insert_symbols.get());

        sqlite3_bind_text(m_insert_symbols.get(), 1, symbol.symbol().c_str(), SQLITE_AUTO_LENGTH, SQLITE_TRANSIENT);
        sqlite3_bind_text(m_insert_symbols.get(), 2, to_exchange(symbol.exchange()).c_str(), SQLITE_AUTO_LENGTH, SQLITE_TRANSIENT);

        m_exec_code = sqlite3_step(m_insert_symbols.get());
        if (m_exec_code != SQLITE_DONE) {
            logger->error("Failed to insert symbol {}: {}", symbol.symbol(), std::string(sqlite3_errmsg(m_db.get())));
        }
    }

    commit_or_rollback(SQLITE_OK);

    return symbols->symbols_size();
}

std::shared_ptr<trade::types::Symbols> trade::holder::SQLiteHolder::query_symbols_by_symbol(const std::string& symbol)
{
    return query_symbols_by("symbol", symbol);
}

std::shared_ptr<trade::types::Symbols> trade::holder::SQLiteHolder::query_symbols_by_exchange(const types::ExchangeType exchange)
{
    return query_symbols_by("exchange", to_exchange(exchange));
}

int64_t trade::holder::SQLiteHolder::update_funds(const std::shared_ptr<types::Funds> funds)
{
    start_transaction();

    for (const auto& fund : funds->funds()) {
        sqlite3_reset(m_insert_funds.get());

        sqlite3_bind_text(m_insert_funds.get(), 1, fund.account_id().c_str(), SQLITE_AUTO_LENGTH, SQLITE_TRANSIENT);
        sqlite3_bind_double(m_insert_funds.get(), 2, fund.available_fund());
        sqlite3_bind_double(m_insert_funds.get(), 3, fund.withdrawn_fund());
        sqlite3_bind_double(m_insert_funds.get(), 4, fund.frozen_fund());
        sqlite3_bind_double(m_insert_funds.get(), 5, fund.frozen_margin());
        sqlite3_bind_double(m_insert_funds.get(), 6, fund.frozen_commission());

        m_exec_code = sqlite3_step(m_insert_funds.get());
        if (m_exec_code != SQLITE_DONE) {
            logger->error("Failed to insert fund {}: {}", fund.account_id(), std::string(sqlite3_errmsg(m_db.get())));
        }
    }

    commit_or_rollback(SQLITE_OK);

    return funds->funds_size();
}

std::shared_ptr<trade::types::Funds> trade::holder::SQLiteHolder::query_funds_by_account_id(const std::string& account_id)
{
    start_transaction();

    sqlite3_reset(m_query_funds_by_account_id.get());

    sqlite3_bind_text(m_query_funds_by_account_id.get(), 1, account_id.c_str(), SQLITE_AUTO_LENGTH, SQLITE_TRANSIENT);

    const auto funds = std::make_shared<types::Funds>();

    while (sqlite3_step(m_query_funds_by_account_id.get()) == SQLITE_ROW) {
        types::Fund fund;

        fund.set_account_id(std::string(reinterpret_cast<const char*>(sqlite3_column_text(m_query_funds_by_account_id.get(), 0))));
        fund.set_available_fund(sqlite3_column_double(m_query_funds_by_account_id.get(), 1));
        fund.set_withdrawn_fund(sqlite3_column_double(m_query_funds_by_account_id.get(), 2));
        fund.set_frozen_fund(sqlite3_column_double(m_query_funds_by_account_id.get(), 3));
        fund.set_frozen_margin(sqlite3_column_double(m_query_funds_by_account_id.get(), 4));
        fund.set_frozen_commission(sqlite3_column_double(m_query_funds_by_account_id.get(), 5));

        funds->add_funds()->CopyFrom(fund);
    }

    commit_or_rollback(SQLITE_OK);

    return funds;
}

int64_t trade::holder::SQLiteHolder::update_positions(const std::shared_ptr<types::Positions> positions)
{
    start_transaction();

    for (const auto& position : positions->positions()) {
        sqlite3_reset(m_insert_positions.get());

        sqlite3_bind_text(m_insert_positions.get(), 1, position.symbol().c_str(), SQLITE_AUTO_LENGTH, SQLITE_TRANSIENT);
        sqlite3_bind_int64(m_insert_positions.get(), 2, position.yesterday_position());
        sqlite3_bind_int64(m_insert_positions.get(), 3, position.today_position());
        sqlite3_bind_int64(m_insert_positions.get(), 4, position.open_volume());
        sqlite3_bind_int64(m_insert_positions.get(), 5, position.close_volume());
        sqlite3_bind_double(m_insert_positions.get(), 6, position.position_cost());
        sqlite3_bind_double(m_insert_positions.get(), 7, position.pre_margin());
        sqlite3_bind_double(m_insert_positions.get(), 8, position.used_margin());
        sqlite3_bind_double(m_insert_positions.get(), 9, position.frozen_margin());
        sqlite3_bind_double(m_insert_positions.get(), 10, position.open_cost());
        sqlite3_bind_text(m_insert_positions.get(), 11, utilities::ToTime<std::string>()(position.update_time()).c_str(), SQLITE_AUTO_LENGTH, SQLITE_TRANSIENT);

        m_exec_code = sqlite3_step(m_insert_positions.get());
        if (m_exec_code != SQLITE_DONE) {
            logger->error("Failed to insert position {}: {}", position.symbol(), std::string(sqlite3_errmsg(m_db.get())));
        }
    }

    commit_or_rollback(SQLITE_OK);

    return positions->positions_size();
}

std::shared_ptr<trade::types::Positions> trade::holder::SQLiteHolder::query_positions_by_symbol(const std::string& symbol)
{
    start_transaction();

    sqlite3_reset(m_query_positions_by_symbol.get());

    sqlite3_bind_text(m_query_positions_by_symbol.get(), 1, symbol.c_str(), SQLITE_AUTO_LENGTH, SQLITE_TRANSIENT);

    const auto positions = std::make_shared<types::Positions>();

    while (sqlite3_step(m_query_positions_by_symbol.get()) == SQLITE_ROW) {
        types::Position position;

        position.set_symbol(std::string(reinterpret_cast<const char*>(sqlite3_column_text(m_query_positions_by_symbol.get(), 0))));
        position.set_yesterday_position(sqlite3_column_int64(m_query_positions_by_symbol.get(), 1));
        position.set_today_position(sqlite3_column_int64(m_query_positions_by_symbol.get(), 2));
        position.set_open_volume(sqlite3_column_int64(m_query_positions_by_symbol.get(), 3));
        position.set_close_volume(sqlite3_column_int64(m_query_positions_by_symbol.get(), 4));
        position.set_position_cost(sqlite3_column_double(m_query_positions_by_symbol.get(), 5));
        position.set_pre_margin(sqlite3_column_double(m_query_positions_by_symbol.get(), 6));
        position.set_used_margin(sqlite3_column_double(m_query_positions_by_symbol.get(), 7));
        position.set_frozen_margin(sqlite3_column_double(m_query_positions_by_symbol.get(), 8));
        position.set_open_cost(sqlite3_column_double(m_query_positions_by_symbol.get(), 9));
        position.set_allocated_update_time(utilities::ToTime<google::protobuf::Timestamp*>()(reinterpret_cast<const char*>(sqlite3_column_text(m_query_positions_by_symbol.get(), 10))));

        positions->add_positions()->CopyFrom(position);
    }

    commit_or_rollback(SQLITE_OK);

    return positions;
}

int64_t trade::holder::SQLiteHolder::update_orders(const std::shared_ptr<types::Orders> orders)
{
    start_transaction();

    for (const auto& order : orders->orders()) {
        if (order.unique_id() == INVALID_ID) {
            logger->warn("No Unique ID for order: {}", utilities::ToJSON()(order));
            continue;
        }

        sqlite3_reset(m_insert_orders.get());

        sqlite3_bind_int64(m_insert_orders.get(), 1, order.unique_id());
        order.has_broker_id() ? sqlite3_bind_text(m_insert_orders.get(), 2, order.broker_id().c_str(), SQLITE_AUTO_LENGTH, SQLITE_TRANSIENT)
                              : sqlite3_bind_null(m_insert_orders.get(), 2);
        order.has_exchange_id() ? sqlite3_bind_text(m_insert_orders.get(), 3, order.exchange_id().c_str(), SQLITE_AUTO_LENGTH, SQLITE_TRANSIENT)
                                : sqlite3_bind_null(m_insert_orders.get(), 3);
        sqlite3_bind_text(m_insert_orders.get(), 4, order.symbol().c_str(), SQLITE_AUTO_LENGTH, SQLITE_TRANSIENT);
        sqlite3_bind_text(m_insert_orders.get(), 5, to_side(order.side()).c_str(), SQLITE_AUTO_LENGTH, SQLITE_TRANSIENT);
        sqlite3_bind_text(m_insert_orders.get(), 6, order.has_position_side() ? to_position_side(order.position_side()).c_str() : "NULL", SQLITE_AUTO_LENGTH, SQLITE_TRANSIENT);
        sqlite3_bind_double(m_insert_orders.get(), 7, order.price());
        sqlite3_bind_int64(m_insert_orders.get(), 8, order.quantity());
        sqlite3_bind_text(m_insert_orders.get(), 9, utilities::ToTime<std::string>()(order.creation_time()).c_str(), SQLITE_AUTO_LENGTH, SQLITE_TRANSIENT);
        sqlite3_bind_text(m_insert_orders.get(), 10, utilities::ToTime<std::string>()(order.update_time()).c_str(), SQLITE_AUTO_LENGTH, SQLITE_TRANSIENT);

        m_exec_code = sqlite3_step(m_insert_orders.get());
        if (m_exec_code != SQLITE_DONE) {
            logger->error("Failed to insert order {}: {}", order.unique_id(), std::string(sqlite3_errmsg(m_db.get())));
        }
    }

    commit_or_rollback(SQLITE_OK);

    return orders->orders_size();
}

std::shared_ptr<trade::types::Orders> trade::holder::SQLiteHolder::query_orders_by_unique_id(const int64_t unique_id)
{
    return query_orders_by("unique_id", std::to_string(unique_id));
}

std::shared_ptr<trade::types::Orders> trade::holder::SQLiteHolder::query_orders_by_broker_id(const std::string& broker_id)
{
    return query_orders_by("broker_id", broker_id);
}

std::shared_ptr<trade::types::Orders> trade::holder::SQLiteHolder::query_orders_by_exchange_id(const std::string& exchange_id)
{
    return query_orders_by("exchange_id", exchange_id);
}

void trade::holder::SQLiteHolder::start_transaction() const
{
    sqlite3_exec(m_db.get(), "BEGIN TRANSACTION;", nullptr, nullptr, nullptr);
}

void trade::holder::SQLiteHolder::commit_or_rollback(const decltype(SQLITE_OK) code) const
{
    if (code == SQLITE_OK) {
        sqlite3_exec(m_db.get(), "COMMIT;", nullptr, nullptr, nullptr);
    }
    else {
        logger->error("Failed to execute SQL: {}", std::string(sqlite3_errmsg(m_db.get())));
        sqlite3_exec(m_db.get(), "ROLLBACK;", nullptr, nullptr, nullptr);
    }
}

void trade::holder::SQLiteHolder::init_symbol_table()
{
    /// Create table for symbols.
    const std::string& create_symbols_table_sql = fmt::format(
        "CREATE TABLE IF NOT EXISTS {} ("
        "symbol   TEXT NOT NULL PRIMARY KEY,"
        "exchange TEXT NOT NULL"
        ");",
        m_symbol_table_name
    );

    start_transaction();

    auto m_exec_code = sqlite3_exec(m_db.get(), create_symbols_table_sql.c_str(), nullptr, nullptr, nullptr);

    commit_or_rollback(m_exec_code);

    /// Prepare insert statement.
    const std::string& symbol_insert_sql = fmt::format(
        "INSERT OR REPLACE INTO {} VALUES (?, ?);",
        m_symbol_table_name
    );

    sqlite3_stmt* raw_insert_symbols;
    m_exec_code = sqlite3_prepare_v2(m_db.get(), symbol_insert_sql.c_str(), SQLITE_AUTO_LENGTH, &raw_insert_symbols, nullptr);

    commit_or_rollback(m_exec_code);

    m_insert_symbols = std::unique_ptr<sqlite3_stmt, SQLite3StmtPtrDeleter>(raw_insert_symbols);

    if (m_exec_code != SQLITE_OK) {
        throw std::runtime_error(fmt::format("Failed to prepare SQL: {}", std::string(sqlite3_errmsg(m_db.get()))));
    }
}

void trade::holder::SQLiteHolder::init_query_symbol_stmts()
{
    m_query_symbols_by["symbol"]   = nullptr;
    m_query_symbols_by["exchange"] = nullptr;

    for (auto& [by, stmt] : m_query_symbols_by) {
        const std::string& sql = fmt::format(
            "SELECT * FROM {} WHERE {} = ?;",
            m_symbol_table_name,
            by
        );

        sqlite3_stmt* raw_stmt;
        const auto m_exec_code = sqlite3_prepare_v2(m_db.get(), sql.c_str(), SQLITE_AUTO_LENGTH, &raw_stmt, nullptr);

        stmt.reset(raw_stmt); /// Hand over ownership to stmt.

        if (m_exec_code != SQLITE_OK) {
            throw std::runtime_error(fmt::format("Failed to prepare SQL: {}", std::string(sqlite3_errmsg(m_db.get()))));
        }
    }
}

std::shared_ptr<trade::types::Symbols> trade::holder::SQLiteHolder::query_symbols_by(const std::string& by, const std::string& id) const
{
    start_transaction();

    sqlite3_stmt* tp_stmt;

    tp_stmt = m_query_symbols_by.at(by).get(); /// If no such by type, throw exception immediately.

    sqlite3_reset(tp_stmt);

    sqlite3_bind_text(tp_stmt, 1, id.c_str(), SQLITE_AUTO_LENGTH, SQLITE_TRANSIENT);

    const auto symbols = std::make_shared<types::Symbols>();

    while (sqlite3_step(tp_stmt) == SQLITE_ROW) {
        types::Symbol symbol;

        symbol.set_symbol(std::string(reinterpret_cast<const char*>(sqlite3_column_text(tp_stmt, 0))));
        symbol.set_exchange(to_exchange(std::string(reinterpret_cast<const char*>(sqlite3_column_text(tp_stmt, 1)))));

        symbols->add_symbols()->CopyFrom(symbol);
    }

    commit_or_rollback(SQLITE_OK);

    return symbols;
}

void trade::holder::SQLiteHolder::init_fund_table()
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

    start_transaction();

    auto m_exec_code = sqlite3_exec(m_db.get(), create_funds_table_sql.c_str(), nullptr, nullptr, nullptr);

    commit_or_rollback(m_exec_code);

    /// Prepare insert statement.
    const std::string& fund_insert_sql = fmt::format(
        "INSERT OR REPLACE INTO {} VALUES(?, ?, ?, ?, ?, ?);",
        m_fund_table_name
    );

    sqlite3_stmt* raw_insert_funds;
    m_exec_code = sqlite3_prepare_v2(m_db.get(), fund_insert_sql.c_str(), SQLITE_AUTO_LENGTH, &raw_insert_funds, nullptr);

    m_insert_funds.reset(raw_insert_funds); /// Hand over ownership to m_insert_funds.

    if (m_exec_code != SQLITE_OK) {
        throw std::runtime_error(fmt::format("Failed to prepare SQL: {}", std::string(sqlite3_errmsg(m_db.get()))));
    }
}

void trade::holder::SQLiteHolder::init_query_fund_stmts()
{
    const std::string& sql = fmt::format(
        "SELECT * FROM {} WHERE account_id = ?;",
        m_fund_table_name
    );

    sqlite3_stmt* raw_query_funds_by_account_id;
    m_exec_code = sqlite3_prepare_v2(m_db.get(), sql.c_str(), SQLITE_AUTO_LENGTH, &raw_query_funds_by_account_id, nullptr);

    m_query_funds_by_account_id.reset(raw_query_funds_by_account_id); /// Hand over ownership to m_query_funds_by_account_id.

    if (m_exec_code != SQLITE_OK) {
        throw std::runtime_error(fmt::format("Failed to prepare SQL: {}", std::string(sqlite3_errmsg(m_db.get()))));
    }
}

void trade::holder::SQLiteHolder::init_position_table()
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

    start_transaction();

    auto m_exec_code = sqlite3_exec(m_db.get(), create_positions_table_sql.c_str(), nullptr, nullptr, nullptr);

    commit_or_rollback(m_exec_code);

    /// Prepare insert statement.
    const std::string& position_insert_sql = fmt::format(
        "INSERT OR REPLACE INTO {} VALUES(?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);",
        m_position_table_name
    );

    sqlite3_stmt* raw_insert_positions;
    m_exec_code = sqlite3_prepare_v2(m_db.get(), position_insert_sql.c_str(), SQLITE_AUTO_LENGTH, &raw_insert_positions, nullptr);

    m_insert_positions.reset(raw_insert_positions); /// Hand over ownership to m_insert_positions.

    if (m_exec_code != SQLITE_OK) {
        throw std::runtime_error(fmt::format("Failed to prepare SQL: {}", std::string(sqlite3_errmsg(m_db.get()))));
    }
}

void trade::holder::SQLiteHolder::init_query_position_stmts()
{
    const std::string& sql = fmt::format(
        "SELECT * FROM {} WHERE symbol = ?;",
        m_position_table_name
    );

    sqlite3_stmt* raw_query_positions_by_symbol;
    m_exec_code = sqlite3_prepare_v2(m_db.get(), sql.c_str(), SQLITE_AUTO_LENGTH, &raw_query_positions_by_symbol, nullptr);

    m_query_positions_by_symbol.reset(raw_query_positions_by_symbol); /// Hand over ownership to m_query_positions_by_symbol.

    if (m_exec_code != SQLITE_OK) {
        throw std::runtime_error(fmt::format("Failed to prepare SQL: {}", std::string(sqlite3_errmsg(m_db.get()))));
    }
}

void trade::holder::SQLiteHolder::init_order_table()
{
    /// Create table for orders.
    const std::string& create_orders_table_sql = fmt::format(
        "CREATE TABLE IF NOT EXISTS {0} ("
        "unique_id     INTEGER NOT NULL PRIMARY KEY,"
        "broker_id     TEXT,"
        "exchange_id   TEXT UNIQUE,"
        "symbol        TEXT NOT NULL,"
        "side          TEXT CHECK(side in ('buy', 'sell')) NOT NULL,"
        "position_side TEXT CHECK(position_side in ('open', 'close')),"
        "price         REAL NOT NULL,"
        "quantity      INTEGER NOT NULL,"
        "creation_time DATETIME NOT NULL,"
        "update_time   DATETIME NOT NULL"
        ");"
        "CREATE INDEX IF NOT EXISTS broker_id_index ON {0} (broker_id);"
        "CREATE UNIQUE INDEX IF NOT EXISTS exchange_id_index ON {0} (exchange_id);",
        m_order_table_name
    );

    start_transaction();

    auto m_exec_code = sqlite3_exec(m_db.get(), create_orders_table_sql.c_str(), nullptr, nullptr, nullptr);

    commit_or_rollback(m_exec_code);

    /// Prepare insert statement.
    const std::string& order_insert_sql = fmt::format(
        "INSERT OR REPLACE INTO {} VALUES(?, ?, ?, ?, ?, ?, ?, ?, ?, ?);",
        m_order_table_name
    );

    sqlite3_stmt* raw_insert_orders;
    m_exec_code = sqlite3_prepare_v2(m_db.get(), order_insert_sql.c_str(), SQLITE_AUTO_LENGTH, &raw_insert_orders, nullptr);

    m_insert_orders.reset(raw_insert_orders); /// Hand over ownership to m_insert_orders.

    if (m_exec_code != SQLITE_OK) {
        throw std::runtime_error(fmt::format("Failed to prepare SQL: {}", std::string(sqlite3_errmsg(m_db.get()))));
    }
}

void trade::holder::SQLiteHolder::init_query_order_stmts()
{
    m_query_orders_by["unique_id"]   = nullptr;
    m_query_orders_by["broker_id"]   = nullptr;
    m_query_orders_by["exchange_id"] = nullptr;

    for (auto& [by, stmt] : m_query_orders_by) {
        const std::string& sql = fmt::format(
            "SELECT * FROM {} WHERE {} = ?;",
            m_order_table_name,
            by
        );

        sqlite3_stmt* raw_stmt;
        const auto m_exec_code = sqlite3_prepare_v2(m_db.get(), sql.c_str(), SQLITE_AUTO_LENGTH, &raw_stmt, nullptr);

        stmt.reset(raw_stmt); /// Hand over ownership to stmt.

        if (m_exec_code != SQLITE_OK) {
            throw std::runtime_error(fmt::format("Failed to prepare SQL: {}", std::string(sqlite3_errmsg(m_db.get()))));
        }
    }
}

std::shared_ptr<trade::types::Orders> trade::holder::SQLiteHolder::query_orders_by(
    const std::string& by,
    const std::string& id
) const
{
    start_transaction();

    sqlite3_stmt* tp_stmt;

    tp_stmt = m_query_orders_by.at(by).get(); /// If no such by type, throw exception immediately.

    sqlite3_reset(tp_stmt);

    if (by == "unique_id") /// unique_id in an INTEGER type in SQLite.
        sqlite3_bind_int64(tp_stmt, 1, std::stoll(id));
    else
        sqlite3_bind_text(tp_stmt, 1, id.c_str(), SQLITE_AUTO_LENGTH, SQLITE_TRANSIENT);

    const auto orders = std::make_shared<types::Orders>();

    while (sqlite3_step(tp_stmt) == SQLITE_ROW) {
        types::Order order;

        order.set_unique_id(sqlite3_column_int64(tp_stmt, 0));
        order.set_broker_id(reinterpret_cast<const char*>(sqlite3_column_text(tp_stmt, 1)));
        order.set_exchange_id(reinterpret_cast<const char*>(sqlite3_column_text(tp_stmt, 2)));
        order.set_symbol(reinterpret_cast<const char*>(sqlite3_column_text(tp_stmt, 3)));
        order.set_side(to_side(reinterpret_cast<const char*>(sqlite3_column_text(tp_stmt, 4))));
        order.set_position_side(to_position_side(reinterpret_cast<const char*>(sqlite3_column_text(tp_stmt, 5))));
        order.set_price(sqlite3_column_double(tp_stmt, 6));
        order.set_quantity(sqlite3_column_int64(tp_stmt, 7));
        order.set_allocated_creation_time(utilities::ToTime<google::protobuf::Timestamp*>()(reinterpret_cast<const char*>(sqlite3_column_text(tp_stmt, 8))));
        order.set_allocated_update_time(utilities::ToTime<google::protobuf::Timestamp*>()(reinterpret_cast<const char*>(sqlite3_column_text(tp_stmt, 9))));

        orders->add_orders()->CopyFrom(order);
    }

    commit_or_rollback(SQLITE_OK);

    return orders;
}

void trade::holder::SQLiteHolder::prepare_database()
{
    init_symbol_table();
    init_query_symbol_stmts();

    init_fund_table();
    init_query_fund_stmts();

    init_position_table();
    init_query_position_stmts();

    init_order_table();
    init_query_order_stmts();
}

std::string trade::holder::SQLiteHolder::to_exchange(const types::ExchangeType exchange)
{
    switch (exchange) {
    case types::ExchangeType::bse: return "BSE";
    case types::ExchangeType::cffex: return "CFFEX";
    case types::ExchangeType::cme: return "CME";
    case types::ExchangeType::czce: return "CZCE";
    case types::ExchangeType::dce: return "DCE";
    case types::ExchangeType::gfex: return "GFEX";
    case types::ExchangeType::hkex: return "HKEX";
    case types::ExchangeType::ine: return "INE";
    case types::ExchangeType::nasd: return "NASD";
    case types::ExchangeType::nyse: return "NYSE";
    case types::ExchangeType::shfe: return "SHFE";
    case types::ExchangeType::sse: return "SSE";
    case types::ExchangeType::szse: return "SZSE";
    default: return "";
    }
}

trade::types::ExchangeType trade::holder::SQLiteHolder::to_exchange(const std::string& exchange)
{
    if (exchange == "BSE") return types::ExchangeType::bse;
    if (exchange == "CFFEX") return types::ExchangeType::cffex;
    if (exchange == "CME") return types::ExchangeType::cme;
    if (exchange == "CZCE") return types::ExchangeType::czce;
    if (exchange == "DCE") return types::ExchangeType::dce;
    if (exchange == "GFEX") return types::ExchangeType::gfex;
    if (exchange == "HKEX") return types::ExchangeType::hkex;
    if (exchange == "INE") return types::ExchangeType::ine;
    if (exchange == "NASD") return types::ExchangeType::nasd;
    if (exchange == "NYSE") return types::ExchangeType::nyse;
    if (exchange == "SHFE") return types::ExchangeType::shfe;
    if (exchange == "SSE") return types::ExchangeType::sse;
    if (exchange == "SZSE") return types::ExchangeType::szse;
    return types::ExchangeType::invalid_exchange;
}

std::string trade::holder::SQLiteHolder::to_side(const types::SideType side)
{
    switch (side) {
    case types::SideType::buy: return "buy";
    case types::SideType::sell: return "sell";
    default: return "";
    }
}

trade::types::SideType trade::holder::SQLiteHolder::to_side(const std::string& side)
{
    if (side == "buy") return types::SideType::buy;
    if (side == "sell") return types::SideType::sell;
    return types::SideType::invalid_side;
}

std::string trade::holder::SQLiteHolder::to_position_side(const types::PositionSideType position_side)
{
    switch (position_side) {
    case types::PositionSideType::open: return "open";
    case types::PositionSideType::close: return "close";
    default: return "";
    }
}

trade::types::PositionSideType trade::holder::SQLiteHolder::to_position_side(const std::string& position_side)
{
    if (position_side == "open") return types::PositionSideType::open;
    if (position_side == "close") return types::PositionSideType::close;
    return types::PositionSideType::invalid_position_side;
}
