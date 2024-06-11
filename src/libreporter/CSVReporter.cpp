#include <filesystem>

#include "libreporter/CSVReporter.h"
#include "utilities/TimeHelper.hpp"

void trade::reporter::CSVReporter::md_trade_generated(const std::shared_ptr<types::MdTrade> md_trade)
{
    new_md_trade_writer(md_trade->symbol());

    m_md_trade_writers[md_trade->symbol()] << fmt::format(
        "{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{}\n",
        md_trade->symbol(),
        md_trade->price(),
        md_trade->quantity(),
        md_trade->sell_10(),
        md_trade->sell_9(),
        md_trade->sell_8(),
        md_trade->sell_7(),
        md_trade->sell_6(),
        md_trade->sell_5(),
        md_trade->sell_4(),
        md_trade->sell_3(),
        md_trade->sell_2(),
        md_trade->sell_1(),
        md_trade->buy_1(),
        md_trade->buy_2(),
        md_trade->buy_3(),
        md_trade->buy_4(),
        md_trade->buy_5(),
        md_trade->buy_6(),
        md_trade->buy_7(),
        md_trade->buy_8(),
        md_trade->buy_9(),
        md_trade->buy_10(),
        /// Time.
        utilities::Now<std::string>()()
    );

    m_outside->md_trade_generated(md_trade);
}

void trade::reporter::CSVReporter::new_md_trade_writer(const std::string& symbol)
{
    if (!m_md_trade_writers.contains(symbol)) [[unlikely]] {
        const std::filesystem::path file_path = fmt::format("{}/{}/{}.csv", m_output_folder, "md_trade", symbol);

        create_directories(std::filesystem::path(file_path).parent_path());
        m_md_trade_writers.emplace(symbol, std::ofstream(file_path));

        logger->info("Opened new md trade writer at {}", file_path.string());

        m_md_trade_writers[symbol]
            << "symbol"
            << "price"
            << "quantity"
            << "sell_10"
            << "sell_9"
            << "sell_8"
            << "sell_7"
            << "sell_6"
            << "sell_5"
            << "sell_4"
            << "sell_3"
            << "sell_2"
            << "sell_1"
            << "buy_1"
            << "buy_2"
            << "buy_3"
            << "buy_4"
            << "buy_5"
            << "buy_6"
            << "buy_7"
            << "buy_8"
            << "buy_9"
            << "buy_10"
            /// Time.
            << "time"
            << std::endl;
    }
}
