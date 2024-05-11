#include <filesystem>
#include <fmt/format.h>
#include <google/protobuf/util/json_util.h>

#include "libholder/SQLLiteHolder.h"
#include "utilities/TimeHelper.hpp"
#include "utilities/ToJSON.hpp"

trade::holder::SQLLiteHolder::SQLLiteHolder(const std::string& db_path)
    : AppBase("SQLLiteHolder"),
      m_db(nullptr),
      m_exec_code(SQLITE_OK),
      m_fund_table_name("funds"),
      m_fund_insert_stmt(nullptr),
      m_position_table_name("positions"),
      m_position_insert_stmt(nullptr),
      m_order_table_name("orders"),
      m_order_insert_stmt(nullptr)
{
    /// Create parent folder if not exist.
    if (db_path != ":memory:") {
        const std::filesystem::path path = db_path;
        if (!exists(path.parent_path())) {
            create_directories(path.parent_path());
        }
    }

    /// Open database.
    sqlite3_open(db_path.c_str(), &m_db);
    if (m_db == nullptr) {
        throw std::runtime_error(fmt::format("Failed to open sqlite3 database: {}", std::string(sqlite3_errmsg(m_db))));
    }

    m_exec_code = sqlite3_exec(m_db, "PRAGMA foreign_keys = ON;", nullptr, nullptr, nullptr);
    if (m_exec_code != SQLITE_OK) {
        throw std::runtime_error(fmt::format("Failed to enable foreign keys: {}", std::string(sqlite3_errmsg(m_db))));
    }

    init_database();
}

trade::holder::SQLLiteHolder::~SQLLiteHolder()
{
    sqlite3_close(m_db);
}

int64_t trade::holder::SQLLiteHolder::update_funds(const std::shared_ptr<types::Funds> funds)
{
    start_transaction();

    for (const auto& fund : funds->funds()) {
        sqlite3_reset(m_fund_insert_stmt);

        sqlite3_bind_text(m_fund_insert_stmt, 1, fund.account_id().c_str(), SQLITE_AUTO_LENGTH, SQLITE_TRANSIENT);
        sqlite3_bind_double(m_fund_insert_stmt, 2, fund.available_fund());
        sqlite3_bind_double(m_fund_insert_stmt, 3, fund.withdrawn_fund());
        sqlite3_bind_double(m_fund_insert_stmt, 4, fund.frozen_fund());
        sqlite3_bind_double(m_fund_insert_stmt, 5, fund.frozen_margin());
        sqlite3_bind_double(m_fund_insert_stmt, 6, fund.frozen_commission());

        m_exec_code = sqlite3_step(m_fund_insert_stmt);
        if (m_exec_code != SQLITE_DONE) {
            logger->error("Failed to insert fund {}: {}", fund.account_id(), std::string(sqlite3_errmsg(m_db)));
        }
    }

    commit_or_rollback(SQLITE_OK);

    return funds->funds_size();
}

std::shared_ptr<trade::types::Funds> trade::holder::SQLLiteHolder::query_funds_by_account_id(const std::string& account_id)
{
    start_transaction();

    sqlite3_reset(m_query_fund_by_account_id_stmt);

    sqlite3_bind_text(m_query_fund_by_account_id_stmt, 1, account_id.c_str(), SQLITE_AUTO_LENGTH, SQLITE_TRANSIENT);

    sqlite3_step(m_query_fund_by_account_id_stmt);

    const auto funds = std::make_shared<types::Funds>();

    while (sqlite3_step(m_query_fund_by_account_id_stmt) == SQLITE_ROW) {
        types::Fund fund;

        fund.set_account_id(std::string(reinterpret_cast<const char*>(sqlite3_column_text(m_query_fund_by_account_id_stmt, 0))));
        fund.set_available_fund(sqlite3_column_double(m_query_fund_by_account_id_stmt, 1));
        fund.set_withdrawn_fund(sqlite3_column_double(m_query_fund_by_account_id_stmt, 2));
        fund.set_frozen_fund(sqlite3_column_double(m_query_fund_by_account_id_stmt, 3));
        fund.set_frozen_margin(sqlite3_column_double(m_query_fund_by_account_id_stmt, 4));
        fund.set_frozen_commission(sqlite3_column_double(m_query_fund_by_account_id_stmt, 5));

        funds->add_funds()->CopyFrom(fund);
    }

    return funds;
}

int64_t trade::holder::SQLLiteHolder::init_positions(const std::shared_ptr<types::Positions> positions)
{
    start_transaction();

    for (const auto& position : positions->positions()) {
        sqlite3_reset(m_position_insert_stmt);

        sqlite3_bind_text(m_position_insert_stmt, 1, position.symbol().c_str(), SQLITE_AUTO_LENGTH, SQLITE_TRANSIENT);
        sqlite3_bind_int64(m_position_insert_stmt, 2, position.yesterday_position());
        sqlite3_bind_int64(m_position_insert_stmt, 3, position.today_position());
        sqlite3_bind_int64(m_position_insert_stmt, 4, position.open_volume());
        sqlite3_bind_int64(m_position_insert_stmt, 5, position.close_volume());
        sqlite3_bind_double(m_position_insert_stmt, 6, position.position_cost());
        sqlite3_bind_double(m_position_insert_stmt, 7, position.pre_margin());
        sqlite3_bind_double(m_position_insert_stmt, 8, position.used_margin());
        sqlite3_bind_double(m_position_insert_stmt, 9, position.frozen_margin());
        sqlite3_bind_double(m_position_insert_stmt, 10, position.open_cost());
        sqlite3_bind_text(m_position_insert_stmt, 11, utilities::ToTime<std::string>()(position.update_time()).c_str(), SQLITE_AUTO_LENGTH, SQLITE_TRANSIENT);

        m_exec_code = sqlite3_step(m_position_insert_stmt);
        if (m_exec_code != SQLITE_DONE) {
            logger->error("Failed to insert position {}: {}", position.symbol(), std::string(sqlite3_errmsg(m_db)));
        }
    }

    commit_or_rollback(SQLITE_OK);

    return positions->positions_size();
}

int64_t trade::holder::SQLLiteHolder::update_orders(const std::shared_ptr<types::Orders> orders)
{
    start_transaction();

    for (const auto& order : orders->orders()) {
        if (order.unique_id() == INVALID_ID) {
            logger->warn("No Unique ID for order: {}", utilities::ToJSON()(order));
            continue;
        }

        sqlite3_reset(m_order_insert_stmt);

        sqlite3_bind_int64(m_order_insert_stmt, 1, order.unique_id());
        order.has_broker_id() ? sqlite3_bind_text(m_order_insert_stmt, 2, order.broker_id().c_str(), SQLITE_AUTO_LENGTH, SQLITE_TRANSIENT)
                              : sqlite3_bind_null(m_order_insert_stmt, 2);
        order.has_exchange_id() ? sqlite3_bind_text(m_order_insert_stmt, 3, order.exchange_id().c_str(), SQLITE_AUTO_LENGTH, SQLITE_TRANSIENT)
                                : sqlite3_bind_null(m_order_insert_stmt, 3);
        sqlite3_bind_text(m_order_insert_stmt, 4, order.symbol().c_str(), SQLITE_AUTO_LENGTH, SQLITE_TRANSIENT);
        sqlite3_bind_text(m_order_insert_stmt, 5, to_side(order.side()).c_str(), SQLITE_AUTO_LENGTH, SQLITE_TRANSIENT);
        sqlite3_bind_text(m_order_insert_stmt, 6, order.has_position_side() ? to_position_side(order.position_side()).c_str() : "NULL", SQLITE_AUTO_LENGTH, SQLITE_TRANSIENT);
        sqlite3_bind_double(m_order_insert_stmt, 7, order.price());
        sqlite3_bind_int64(m_order_insert_stmt, 8, order.quantity());
        sqlite3_bind_text(m_order_insert_stmt, 9, utilities::ToTime<std::string>()(order.creation_time()).c_str(), SQLITE_AUTO_LENGTH, SQLITE_TRANSIENT);
        sqlite3_bind_text(m_order_insert_stmt, 10, utilities::ToTime<std::string>()(order.update_time()).c_str(), SQLITE_AUTO_LENGTH, SQLITE_TRANSIENT);

        m_exec_code = sqlite3_step(m_order_insert_stmt);
        if (m_exec_code != SQLITE_DONE) {
            logger->error("Failed to insert order {}: {}", order.unique_id(), std::string(sqlite3_errmsg(m_db)));
        }
    }

    commit_or_rollback(SQLITE_OK);

    return orders->orders_size();
}

std::shared_ptr<trade::types::Orders> trade::holder::SQLLiteHolder::query_orders_by_unique_id(int64_t unique_id)
{
    return query_orders_by("unique_id", std::to_string(unique_id));
}

std::shared_ptr<trade::types::Orders> trade::holder::SQLLiteHolder::query_orders_by_broker_id(const std::string& broker_id)
{
    return query_orders_by("broker_id", broker_id);
}

std::shared_ptr<trade::types::Orders> trade::holder::SQLLiteHolder::query_orders_by_exchange_id(const std::string& exchange_id)
{
    return query_orders_by("exchange_id", exchange_id);
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

    start_transaction();

    auto m_exec_code = sqlite3_exec(m_db, create_funds_table_sql.c_str(), nullptr, nullptr, nullptr);

    commit_or_rollback(m_exec_code);

    /// Prepare insert statement.
    const std::string& fund_insert_sql = fmt::format(
        "INSERT OR REPLACE INTO {} VALUES(?, ?, ?, ?, ?, ?);",
        m_fund_table_name
    );

    m_exec_code = sqlite3_prepare_v2(m_db, fund_insert_sql.c_str(), SQLITE_AUTO_LENGTH, &m_fund_insert_stmt, nullptr);

    if (m_exec_code != SQLITE_OK) {
        throw std::runtime_error(fmt::format("Failed to prepare SQL: {}", std::string(sqlite3_errmsg(m_db))));
    }
}

void trade::holder::SQLLiteHolder::init_query_fund_by_account_id_stmt()
{
    const std::string& sql = fmt::format(
        "SELECT * FROM {} WHERE account_id = ?;",
        m_fund_table_name
    );

    m_exec_code = sqlite3_prepare_v2(m_db, sql.c_str(), SQLITE_AUTO_LENGTH, &m_query_fund_by_account_id_stmt, nullptr);

    if (m_exec_code != SQLITE_OK) {
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

    start_transaction();

    auto m_exec_code = sqlite3_exec(m_db, create_positions_table_sql.c_str(), nullptr, nullptr, nullptr);

    commit_or_rollback(m_exec_code);

    /// Prepare insert statement.
    const std::string& position_insert_sql = fmt::format(
        "INSERT OR REPLACE INTO {} VALUES(?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);",
        m_position_table_name
    );

    m_exec_code = sqlite3_prepare_v2(m_db, position_insert_sql.c_str(), SQLITE_AUTO_LENGTH, &m_position_insert_stmt, nullptr);

    if (m_exec_code != SQLITE_OK) {
        throw std::runtime_error(fmt::format("Failed to prepare SQL: {}", std::string(sqlite3_errmsg(m_db))));
    }
}

void trade::holder::SQLLiteHolder::init_orders_table()
{
    /// Create table for orders.
    const std::string& create_orders_table_sql = fmt::format(
        "CREATE TABLE IF NOT EXISTS {0} ("
        "unique_id     TEXT NOT NULL PRIMARY KEY,"
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

    auto m_exec_code = sqlite3_exec(m_db, create_orders_table_sql.c_str(), nullptr, nullptr, nullptr);

    commit_or_rollback(m_exec_code);

    /// Prepare insert statement.
    const std::string& order_insert_sql = fmt::format(
        "INSERT OR REPLACE INTO {} VALUES(?, ?, ?, ?, ?, ?, ?, ?, ?, ?);",
        m_order_table_name
    );

    m_exec_code = sqlite3_prepare_v2(m_db, order_insert_sql.c_str(), SQLITE_AUTO_LENGTH, &m_order_insert_stmt, nullptr);

    if (m_exec_code != SQLITE_OK) {
        throw std::runtime_error(fmt::format("Failed to prepare SQL: {}", std::string(sqlite3_errmsg(m_db))));
    }
}

void trade::holder::SQLLiteHolder::init_query_orders_stmts()
{
    const std::array<std::string, 3> ids = {
        "unique_id",
        "broker_id",
        "exchange_id"
    };

    const std::array<sqlite3_stmt**, 3> stmts = {
        &m_query_orders_by_unique_id_stmt,
        &m_query_orders_by_broker_id_stmt,
        &m_query_orders_by_exchange_id_stmt
    };

    for (size_t i = 0; i < 3; i++) {
        const std::string& sql = fmt::format(
            "SELECT * FROM {} WHERE {} = ?;",
            m_order_table_name,
            ids[i]
        );

        const auto m_exec_code = sqlite3_prepare_v2(m_db, sql.c_str(), SQLITE_AUTO_LENGTH, stmts[i], nullptr);
        if (m_exec_code != SQLITE_OK) {
            throw std::runtime_error(fmt::format("Failed to prepare SQL: {}", std::string(sqlite3_errmsg(m_db))));
        }
    }
}

std::shared_ptr<trade::types::Orders> trade::holder::SQLLiteHolder::query_orders_by(
    const std::string& by,
    const std::string& id
) const
{
    start_transaction();

    sqlite3_stmt* tp_stmt;

    /// TODO: Optimize this.
    if (by == "unique_id")
        tp_stmt = m_query_orders_by_unique_id_stmt;
    else if (by == "broker_id")
        tp_stmt = m_query_orders_by_broker_id_stmt;
    else if (by == "exchange_id")
        tp_stmt = m_query_orders_by_exchange_id_stmt;
    else
        throw std::runtime_error(fmt::format("Invalid query type: {}", by));

    sqlite3_reset(tp_stmt);

    sqlite3_bind_text(tp_stmt, 1, id.c_str(), SQLITE_AUTO_LENGTH, SQLITE_TRANSIENT);

    sqlite3_step(tp_stmt);

    const auto orders = std::make_shared<types::Orders>();

    while (sqlite3_step(tp_stmt) == SQLITE_ROW) {
        types::Order order;

        order.set_unique_id(sqlite3_column_int64(tp_stmt, 1));
        order.set_broker_id(reinterpret_cast<const char*>(sqlite3_column_text(tp_stmt, 2)));
        order.set_exchange_id(reinterpret_cast<const char*>(sqlite3_column_text(tp_stmt, 3)));
        order.set_symbol(reinterpret_cast<const char*>(sqlite3_column_text(tp_stmt, 4)));
        order.set_side(to_side(reinterpret_cast<const char*>(sqlite3_column_text(tp_stmt, 5))));
        order.set_position_side(to_position_side(reinterpret_cast<const char*>(sqlite3_column_text(tp_stmt, 6))));
        order.set_price(sqlite3_column_double(tp_stmt, 7));
        order.set_quantity(sqlite3_column_int64(tp_stmt, 8));
        order.set_allocated_creation_time(utilities::ToTime<google::protobuf::Timestamp*>()(reinterpret_cast<const char*>(sqlite3_column_text(tp_stmt, 9))));

        orders->add_orders()->CopyFrom(order);
    }

    return orders;
}

void trade::holder::SQLLiteHolder::init_database()
{
    init_funds_table();
    init_positions_table();
    init_orders_table();

    init_query_orders_stmts();
}

std::string trade::holder::SQLLiteHolder::to_exchange(types::ExchangeType exchange)
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

trade::types::ExchangeType trade::holder::SQLLiteHolder::to_exchange(const std::string& exchange)
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

std::string trade::holder::SQLLiteHolder::to_side(const types::SideType side)
{
    switch (side) {
    case types::SideType::buy: return "buy";
    case types::SideType::sell: return "sell";
    default: return "";
    }
}

trade::types::SideType trade::holder::SQLLiteHolder::to_side(const std::string& side)
{
    if (side == "buy") return types::SideType::buy;
    if (side == "sell") return types::SideType::sell;
    return types::SideType::invalid_side;
}

std::string trade::holder::SQLLiteHolder::to_position_side(const types::PositionSideType position_side)
{
    switch (position_side) {
    case types::PositionSideType::open: return "open";
    case types::PositionSideType::close: return "close";
    default: return "";
    }
}

trade::types::PositionSideType trade::holder::SQLLiteHolder::to_position_side(const std::string& position_side)
{
    if (position_side == "open") return types::PositionSideType::open;
    if (position_side == "close") return types::PositionSideType::close;
    return types::PositionSideType::invalid_position_side;
}
