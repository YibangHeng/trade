#include <filesystem>
#include <ranges>

#include "libreporter/CSVReporter.h"
#include "utilities/TimeHelper.hpp"

trade::reporter::CSVReporter::~CSVReporter()
{
    /// Flush all writers.
    for (auto& writer : m_l2_tick_writers | std::views::values) {
        writer.flush();
        writer.close();
    }
    for (auto& writer : m_ranged_tick_writers | std::views::values) {
        writer.flush();
        writer.close();
    }
}

void trade::reporter::CSVReporter::l2_tick_generated(const std::shared_ptr<types::GeneratedL2Tick> generated_l2_tick)
{
    new_l2_tick_writer(generated_l2_tick->symbol());

    m_l2_tick_writers[generated_l2_tick->symbol()] << fmt::format(
        "{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{}\n",
        generated_l2_tick->symbol(),
        generated_l2_tick->price_1000x(),
        generated_l2_tick->quantity(),
        generated_l2_tick->ask_unique_id(),
        generated_l2_tick->bid_unique_id(),
        generated_l2_tick->ask_levels().at(4).price_1000x(),
        generated_l2_tick->ask_levels().at(4).quantity(),
        generated_l2_tick->ask_levels().at(3).price_1000x(),
        generated_l2_tick->ask_levels().at(3).quantity(),
        generated_l2_tick->ask_levels().at(2).price_1000x(),
        generated_l2_tick->ask_levels().at(2).quantity(),
        generated_l2_tick->ask_levels().at(1).price_1000x(),
        generated_l2_tick->ask_levels().at(1).quantity(),
        generated_l2_tick->ask_levels().at(0).price_1000x(),
        generated_l2_tick->ask_levels().at(0).quantity(),
        generated_l2_tick->bid_levels().at(0).price_1000x(),
        generated_l2_tick->bid_levels().at(0).quantity(),
        generated_l2_tick->bid_levels().at(1).price_1000x(),
        generated_l2_tick->bid_levels().at(1).quantity(),
        generated_l2_tick->bid_levels().at(2).price_1000x(),
        generated_l2_tick->bid_levels().at(2).quantity(),
        generated_l2_tick->bid_levels().at(3).price_1000x(),
        generated_l2_tick->bid_levels().at(3).quantity(),
        generated_l2_tick->bid_levels().at(4).price_1000x(),
        generated_l2_tick->bid_levels().at(4).quantity(),
        /// Time.
        generated_l2_tick->exchange_time(),
        utilities::Now<std::string>()()
    );

    m_outside->l2_tick_generated(generated_l2_tick);
}

void trade::reporter::CSVReporter::ranged_tick_generated(const std::shared_ptr<types::RangedTick> ranged_tick)
{
    new_ranged_tick_writer(ranged_tick->symbol());

    m_ranged_tick_writers[ranged_tick->symbol()] << fmt::format(
        "{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{}\n",
        ranged_tick->symbol(),
        ranged_tick->start_time(),
        ranged_tick->end_time(),
        ranged_tick->exchange_time(),
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
    );
}

void trade::reporter::CSVReporter::new_l2_tick_writer(const std::string& symbol)
{
    if (!m_l2_tick_writers.contains(symbol)) [[unlikely]] {
        const std::filesystem::path file_path = fmt::format("{}/{}/{}-l2-tick.csv", m_output_folder, utilities::Date<std::string>()(), symbol);

        create_directories(std::filesystem::path(file_path).parent_path());
        m_l2_tick_writers.emplace(symbol, std::ofstream(file_path));

        logger->info("Opened new l2 tick writer at {}", file_path.string());

        m_l2_tick_writers[symbol]
            << "symbol,"
            << "price_1000x,"
            << "quantity,"
            << "ask_unique_id,"
            << "bid_unique_id,"
            << "sell_price_1000x_5,"
            << "sell_quantity_5,"
            << "sell_price_1000x_4,"
            << "sell_quantity_4,"
            << "sell_price_1000x_3,"
            << "sell_quantity_3,"
            << "sell_price_1000x_2,"
            << "sell_quantity_2,"
            << "sell_price_1000x_1,"
            << "sell_quantity_1,"
            << "buy_price_1000x_1,"
            << "buy_quantity_1,"
            << "buy_price_1000x_2,"
            << "buy_quantity_2,"
            << "buy_price_1000x_3,"
            << "buy_quantity_3,"
            << "buy_price_1000x_4,"
            << "buy_quantity_4,"
            << "buy_price_1000x_5,"
            << "buy_quantity_5,"
            /// Time.
            << "exchange_time,"
            << "local_system_time"
            << std::endl;
    }
}

void trade::reporter::CSVReporter::new_ranged_tick_writer(const std::string& symbol)
{
    if (!m_ranged_tick_writers.contains(symbol)) [[unlikely]] {
        const std::filesystem::path file_path = fmt::format("{}/{}/{}-ranged-tick.csv", m_output_folder, utilities::Date<std::string>()(), symbol);

        create_directories(std::filesystem::path(file_path).parent_path());
        m_ranged_tick_writers.emplace(symbol, std::ofstream(file_path));

        logger->info("Opened new ranged tick writer at {}", file_path.string());

        m_ranged_tick_writers[symbol]
            << "symbol,"
            << "start_time,"
            << "end_time,"
            << "exchange_time,"
            << "sell_price_1000x_5,"
            << "sell_price_1000x_4,"
            << "sell_price_1000x_3,"
            << "sell_price_1000x_2,"
            << "sell_price_1000x_1,"
            << "buy_price_1000x_1,"
            << "buy_price_1000x_2,"
            << "buy_price_1000x_3,"
            << "buy_price_1000x_4,"
            << "buy_price_1000x_5,"
            << "active_traded_sell_number,"
            << "active_sell_number,"
            << "active_sell_quantity,"
            << "active_sell_amount_1000x,"
            << "active_traded_buy_number,"
            << "active_buy_number,"
            << "active_buy_quantity,"
            << "active_buy_amount_1000x,"
            << "weighted_ask_price_5,"
            << "weighted_ask_price_4,"
            << "weighted_ask_price_3,"
            << "weighted_ask_price_2,"
            << "weighted_ask_price_1,"
            << "weighted_bid_price_1,"
            << "weighted_bid_price_2,"
            << "weighted_bid_price_3,"
            << "weighted_bid_price_4,"
            << "weighted_bid_price_5,"
            << "aggressive_sell_number,"
            << "aggressive_buy_number,"
            << "new_added_ask_1_quantity,"
            << "new_added_bid_1_quantity,"
            << "new_canceled_ask_1_quantity,"
            << "new_canceled_bid_1_quantity,"
            << "new_canceled_ask_all_quantity,"
            << "new_canceled_bid_all_quantity,"
            << "big_ask_amount_1000x,"
            << "big_bid_amount_1000x,"
            << "highest_price_1000x,"
            << "lowest_price_1000x,"
            << "ask_price_1_valid_duration_1000x,"
            << "bid_price_1_valid_duration_1000x"
            << std::endl;
    }
}
