#include <filesystem>

#include "libreporter/CSVReporter.h"
#include "utilities/TimeHelper.hpp"

trade::reporter::CSVReporter::~CSVReporter()
{
    /// Flush all writers.
    for (auto& [symbol, writer] : m_l2_tick_writers)
        writer.flush();
}

void trade::reporter::CSVReporter::l2_tick_generated(const std::shared_ptr<types::L2Tick> l2_tick)
{
    new_l2_tick_writer(l2_tick->symbol());

    m_l2_tick_writers[l2_tick->symbol()] << fmt::format(
        "{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{}\n",
        l2_tick->symbol(),
        l2_tick->price(),
        l2_tick->quantity(),
        l2_tick->ask_unique_id(),
        l2_tick->bid_unique_id(),
        l2_tick->sell_price_10(),
        l2_tick->sell_quantity_10(),
        l2_tick->sell_price_9(),
        l2_tick->sell_quantity_9(),
        l2_tick->sell_price_8(),
        l2_tick->sell_quantity_8(),
        l2_tick->sell_price_7(),
        l2_tick->sell_quantity_7(),
        l2_tick->sell_price_6(),
        l2_tick->sell_quantity_6(),
        l2_tick->sell_price_5(),
        l2_tick->sell_quantity_5(),
        l2_tick->sell_price_4(),
        l2_tick->sell_quantity_4(),
        l2_tick->sell_price_3(),
        l2_tick->sell_quantity_3(),
        l2_tick->sell_price_2(),
        l2_tick->sell_quantity_2(),
        l2_tick->sell_price_1(),
        l2_tick->sell_quantity_1(),
        l2_tick->buy_price_1(),
        l2_tick->buy_quantity_1(),
        l2_tick->buy_price_2(),
        l2_tick->buy_quantity_2(),
        l2_tick->buy_price_3(),
        l2_tick->buy_quantity_3(),
        l2_tick->buy_price_4(),
        l2_tick->buy_quantity_4(),
        l2_tick->buy_price_5(),
        l2_tick->buy_quantity_5(),
        l2_tick->buy_price_6(),
        l2_tick->buy_quantity_6(),
        l2_tick->buy_price_7(),
        l2_tick->buy_quantity_7(),
        l2_tick->buy_price_8(),
        l2_tick->buy_quantity_8(),
        l2_tick->buy_price_9(),
        l2_tick->buy_quantity_9(),
        l2_tick->buy_price_10(),
        l2_tick->buy_quantity_10(),
        /// Time.
        l2_tick->exchange_time(),
        utilities::Now<std::string>()()
    );

    m_outside->l2_tick_generated(l2_tick);
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
            << "price,"
            << "quantity,"
            << "ask_unique_id,"
            << "bid_unique_id,"
            << "sell_price_10,"
            << "sell_quantity_10,"
            << "sell_price_9,"
            << "sell_quantity_9,"
            << "sell_price_8,"
            << "sell_quantity_8,"
            << "sell_price_7,"
            << "sell_quantity_7,"
            << "sell_price_6,"
            << "sell_quantity_6,"
            << "sell_price_5,"
            << "sell_quantity_5,"
            << "sell_price_4,"
            << "sell_quantity_4,"
            << "sell_price_3,"
            << "sell_quantity_3,"
            << "sell_price_2,"
            << "sell_quantity_2,"
            << "sell_price_1,"
            << "sell_quantity_1,"
            << "buy_price_1,"
            << "buy_quantity_1,"
            << "buy_price_2,"
            << "buy_quantity_2,"
            << "buy_price_3,"
            << "buy_quantity_3,"
            << "buy_price_4,"
            << "buy_quantity_4,"
            << "buy_price_5,"
            << "buy_quantity_5,"
            << "buy_price_6,"
            << "buy_quantity_6,"
            << "buy_price_7,"
            << "buy_quantity_7,"
            << "buy_price_8,"
            << "buy_quantity_8,"
            << "buy_price_9,"
            << "buy_quantity_9,"
            << "buy_price_10,"
            << "buy_quantity10,"
            /// Time.
            << "exchange_time,"
            << "local_system_time"
            << std::endl;
    }
}
