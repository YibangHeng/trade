#include <filesystem>

#include "libreporter/CSVReporter.h"
#include "utilities/TimeHelper.hpp"

void trade::reporter::CSVReporter::md_trade_generated(const std::shared_ptr<types::MdTrade> md_trade)
{
    new_md_trade_writer(md_trade->symbol());

    m_md_trade_writers[md_trade->symbol()] << fmt::format(
        "{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{}\n",
        md_trade->symbol(),
        md_trade->price(),
        md_trade->quantity(),
        md_trade->sell_price_10(),
        md_trade->sell_quantity_10(),
        md_trade->sell_price_9(),
        md_trade->sell_quantity_9(),
        md_trade->sell_price_8(),
        md_trade->sell_quantity_8(),
        md_trade->sell_price_7(),
        md_trade->sell_quantity_7(),
        md_trade->sell_price_6(),
        md_trade->sell_quantity_6(),
        md_trade->sell_price_5(),
        md_trade->sell_quantity_5(),
        md_trade->sell_price_4(),
        md_trade->sell_quantity_4(),
        md_trade->sell_price_3(),
        md_trade->sell_quantity_3(),
        md_trade->sell_price_2(),
        md_trade->sell_quantity_2(),
        md_trade->sell_price_1(),
        md_trade->sell_quantity_1(),
        md_trade->buy_price_1(),
        md_trade->buy_quantity_1(),
        md_trade->buy_price_2(),
        md_trade->buy_quantity_2(),
        md_trade->buy_price_3(),
        md_trade->buy_quantity_3(),
        md_trade->buy_price_4(),
        md_trade->buy_quantity_4(),
        md_trade->buy_price_5(),
        md_trade->buy_quantity_5(),
        md_trade->buy_price_6(),
        md_trade->buy_quantity_6(),
        md_trade->buy_price_7(),
        md_trade->buy_quantity_7(),
        md_trade->buy_price_8(),
        md_trade->buy_quantity_8(),
        md_trade->buy_price_9(),
        md_trade->buy_quantity_9(),
        md_trade->buy_price_10(),
        md_trade->buy_quantity_10(),
        /// Time.
        utilities::Now<std::string>()()
    );

    m_outside->md_trade_generated(md_trade);
}

void trade::reporter::CSVReporter::new_md_trade_writer(const std::string& symbol)
{
    if (!m_md_trade_writers.contains(symbol)) [[unlikely]] {
        const std::filesystem::path file_path = fmt::format("{}/{}/{}-md-trade.csv", m_output_folder, utilities::Date<std::string>()(), symbol);

        create_directories(std::filesystem::path(file_path).parent_path());
        m_md_trade_writers.emplace(symbol, std::ofstream(file_path));

        logger->info("Opened new md trade writer at {}", file_path.string());

        m_md_trade_writers[symbol]
            << "symbol,"
            << "price,"
            << "quantity,"
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
            << "buy_pquantity10,"
            /// Time.
            << "time"
            << std::endl;
    }
}
