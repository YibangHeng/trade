#include "libreporter/MySQLReporter.h"

trade::reporter::MySQLReporter::MySQLReporter(
    const std::string& url,
    const std::string& user,
    const std::string& password,
    const std::string& database,
    const std::string& table,
    const std::shared_ptr<IReporter>& outside
) : AppBase("MySQLReporter"),
    NopReporter(outside),
    m_outside(outside)
{
    try {
        m_sess   = std::make_shared<mysqlx::Session>(fmt::format("mysqlx://{}:{}@{}?connect-timeout=10", user, password, url));
        m_schema = std::make_shared<mysqlx::Schema>(m_sess->getSchema(database));
        m_table  = std::make_shared<mysqlx::Table>(m_schema->getTable(table));
    }
    catch (const std::exception& e) {
        logger->error("MySQLReporter failed to connect to {} as user {}: {}", url, user, e.what());
        return;
    }

    logger->info("MySQLReporter connected to {} as user {}", url, user);
}

void trade::reporter::MySQLReporter::ranged_tick_generated(const std::shared_ptr<types::RangedTick> ranged_tick)
{
    if (m_table != nullptr)
        m_table->insert(
                   "symbol",
                   "exchange_date",
                   "exchange_time",
                   "start_time",
                   "end_time",
                   "sell_price_1000x_5",
                   "sell_price_1000x_4",
                   "sell_price_1000x_3",
                   "sell_price_1000x_2",
                   "sell_price_1000x_1",
                   "buy_price_1000x_1",
                   "buy_price_1000x_2",
                   "buy_price_1000x_3",
                   "buy_price_1000x_4",
                   "buy_price_1000x_5",
                   "active_traded_sell_number",
                   "active_sell_number",
                   "active_sell_quantity",
                   "active_sell_amount_1000x",
                   "active_traded_buy_number",
                   "active_buy_number",
                   "active_buy_quantity",
                   "active_buy_amount_1000x",
                   "weighted_ask_price_5",
                   "weighted_ask_price_4",
                   "weighted_ask_price_3",
                   "weighted_ask_price_2",
                   "weighted_ask_price_1",
                   "weighted_bid_price_1",
                   "weighted_bid_price_2",
                   "weighted_bid_price_3",
                   "weighted_bid_price_4",
                   "weighted_bid_price_5",
                   "aggressive_sell_number",
                   "aggressive_buy_number",
                   "new_added_ask_1_quantity",
                   "new_added_bid_1_quantity",
                   "new_canceled_ask_1_quantity",
                   "new_canceled_bid_1_quantity",
                   "new_canceled_ask_all_quantity",
                   "new_canceled_bid_all_quantity",
                   "big_ask_amount_1000x",
                   "big_bid_amount_1000x",
                   "highest_price_1000x",
                   "lowest_price_1000x",
                   "ask_price_1_valid_duration_1000x",
                   "bid_price_1_valid_duration_1000x"
        )
            .values(
                ranged_tick->symbol(),
                ranged_tick->exchange_date(),
                ranged_tick->exchange_time(),
                ranged_tick->start_time(),
                ranged_tick->end_time(),
                ranged_tick->ask_levels().at(4).price_1000x(),
                ranged_tick->ask_levels().at(3).price_1000x(),
                ranged_tick->ask_levels().at(2).price_1000x(),
                ranged_tick->ask_levels().at(1).price_1000x(),
                ranged_tick->ask_levels().at(0).price_1000x(),
                ranged_tick->bid_levels().at(0).price_1000x(),
                ranged_tick->bid_levels().at(1).price_1000x(),
                ranged_tick->bid_levels().at(2).price_1000x(),
                ranged_tick->bid_levels().at(3).price_1000x(),
                ranged_tick->bid_levels().at(4).price_1000x(),
                ranged_tick->active_traded_sell_number(),
                ranged_tick->active_sell_number(),
                ranged_tick->active_sell_quantity(),
                ranged_tick->active_sell_amount_1000x(),
                ranged_tick->active_traded_buy_number(),
                ranged_tick->active_buy_number(),
                ranged_tick->active_buy_quantity(),
                ranged_tick->active_buy_amount_1000x(),
                ranged_tick->weighted_ask_price().at(4),
                ranged_tick->weighted_ask_price().at(3),
                ranged_tick->weighted_ask_price().at(2),
                ranged_tick->weighted_ask_price().at(1),
                ranged_tick->weighted_ask_price().at(0),
                ranged_tick->weighted_bid_price().at(0),
                ranged_tick->weighted_bid_price().at(1),
                ranged_tick->weighted_bid_price().at(2),
                ranged_tick->weighted_bid_price().at(3),
                ranged_tick->weighted_bid_price().at(4),
                ranged_tick->aggressive_sell_number(),
                ranged_tick->aggressive_buy_number(),
                ranged_tick->new_added_ask_1_quantity(),
                ranged_tick->new_added_bid_1_quantity(),
                ranged_tick->new_canceled_ask_1_quantity(),
                ranged_tick->new_canceled_bid_1_quantity(),
                ranged_tick->new_canceled_ask_all_quantity(),
                ranged_tick->new_canceled_bid_all_quantity(),
                ranged_tick->big_ask_amount_1000x(),
                ranged_tick->big_bid_amount_1000x(),
                ranged_tick->highest_price_1000x(),
                ranged_tick->lowest_price_1000x(),
                ranged_tick->ask_price_1_valid_duration_1000x(),
                ranged_tick->bid_price_1_valid_duration_1000x()
            )
            .execute();

    m_outside->ranged_tick_generated(ranged_tick);
}

trade::reporter::MySQLReporter::~MySQLReporter()
{
    m_sess != nullptr ? m_sess->close() : void();
}
