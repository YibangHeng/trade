#include <csignal>
#include <fast-cpp-csv-parser/csv.h>
#include <filesystem>
#include <future>
#include <iostream>

#include "auxiliaries/offline_booker/OfflineBooker.h"
#include "enums.pb.h"
#include "info.h"
#include "libreporter/AsyncReporter.h"
#include "libreporter/CSVReporter.h"
#include "libreporter/LogReporter.h"
#include "utilities/MakeAssignable.hpp"
#include "utilities/NetworkHelper.hpp"

trade::OfflineBooker::OfflineBooker(const int argc, char* argv[])
    : AppBase("offline_booker")
{
    m_instances.emplace(this);
    m_is_running = argv_parse(argc, argv);
}

int trade::OfflineBooker::run()
{
    if (!m_is_running) {
        return m_exit_code;
    }

    const auto log_reporter = std::make_shared<reporter::LogReporter>();
    const auto csv_reporter = std::make_shared<reporter::CSVReporter>(m_arguments["l2-output-file"].as<std::string>(), log_reporter);

    /// Reporter.
    m_reporter = csv_reporter;

    /// Booker.
    m_booker = std::make_shared<booker::Booker>(std::vector<std::string> {}, m_reporter, true);

    const std::filesystem::path std_file(m_arguments["std-tick-file"].as<std::string>());

    if (!is_regular_file(std_file)) {
        logger->error("{} is not existent or not a regular file", std_file.string());
        return EXIT_FAILURE;
    }

    const auto future = std::async(std::launch::async, [this, &std_file] {
        load_tick(std_file.string());
    });

    while (true) {
        StdTick* std_tick;

        if (m_pairs.pop(std_tick)) {
            booker(std_tick);
            delete std_tick;
        }
        else if (future.wait_for(std::chrono::seconds(0)) == std::future_status::ready) {
            break;
        }

        if (!m_is_running)
            return EXIT_FAILURE;
    }

    return m_exit_code;
}

int trade::OfflineBooker::stop(int signal)
{
    spdlog::info("App exiting since received signal {}", signal);

    m_is_running = false;

    return 0;
}

void trade::OfflineBooker::signal(const int signal)
{
    switch (signal) {
    case SIGINT:
    case SIGTERM: {
        for (const auto& instance : m_instances) {
            instance->stop(signal);
        }

        break;
    }
    default: {
        spdlog::info("Signal {} omitted", signal);
    }
    }
}

bool trade::OfflineBooker::argv_parse(const int argc, char* argv[])
{
    boost::program_options::options_description desc("Allowed options");

    /// Help and version.
    desc.add_options()("help,h", "print help message");
    desc.add_options()("version,v", "print version string and exit");

    desc.add_options()("debug,d", "enable debug output");

    /// Order/Trade ticks folder and generated l2 output file.
    desc.add_options()("std-tick-file,i", boost::program_options::value<std::string>()->default_value("./data/std_tick"), "standard tick file");
    desc.add_options()("l2-output-file,o", boost::program_options::value<std::string>()->default_value("./output/l2_tick"), "l2 output file");

    try {
        store(parse_command_line(argc, argv, desc), m_arguments);
    }
    catch (const boost::wrapexcept<boost::program_options::unknown_option>& e) {
        std::cout << e.what() << std::endl;
        m_exit_code = EXIT_FAILURE;
        return false;
    }

    notify(m_arguments);

/// contains() is not support on Windows platforms.
#if WIN32
    #define contains(s) count(s) > 1
#endif

    if (m_arguments.contains("help")) {
        std::cout << desc;
        return false;
    }

    if (m_arguments.contains("version")) {
        std::cout << fmt::format("{} {}", app_name(), trade_VERSION) << std::endl;
        return false;
    }

    if (m_arguments.contains("debug")) {
        spdlog::set_level(spdlog::level::debug);
        this->logger->set_level(spdlog::level::debug);
    }

#if WIN32
    #undef contains
#endif

    return true;
}

void trade::OfflineBooker::load_tick(const std::string& path)
{
    io::CSVReader<7> in(path);

    in.read_header(
        io::ignore_extra_column,
        "symbol",
        "ask_unique_id",
        "bid_unique_id",
        "order_type",
        "price",
        "quantity",
        "date",
        "time"
    );

    std::string symbol;
    int64_t ask_unique_id;
    int64_t bid_unique_id;
    char order_type;
    double price;
    int64_t quantity;
    int64_t date;
    int64_t time;

    while (m_is_running
           && in.read_row(
               symbol,
               ask_unique_id,
               bid_unique_id,
               order_type,
               price,
               quantity,
               date,
               time
           )) {
        auto std_tick = new StdTick(
            symbol,
            ask_unique_id,
            bid_unique_id,
            to_order_type(order_type),
            price,
            quantity,
            date,
            time
        );

        while (!m_pairs.push(std_tick))
            ;
    }
}

void trade::OfflineBooker::booker(StdTick* std_tick)
{
    const booker::OrderTickPtr order_tick = to_order_tick(std_tick);
    const booker::TradeTickPtr trade_tick = to_trade_tick(std_tick);

    if (order_tick == nullptr && trade_tick == nullptr) {
        logger->error("Invalid tick fed: neither order nor trade tick");
        return;
    }

    if (order_tick != nullptr && trade_tick != nullptr) {
        logger->error("Invalid tick fed: both order and trade tick");
        return;
    }

    if (order_tick != nullptr) {
        if (order_tick->exchange_time() >= 93000000) [[likely]]
            m_booker->switch_to_continuous_stage();

        m_booker->add(order_tick);

        m_reporter->exchange_order_tick_arrived(order_tick);
    }

    if (trade_tick != nullptr) {
        if (trade_tick->exchange_time() >= 93000000) [[likely]]
            m_booker->switch_to_continuous_stage();

        const auto verification_passed = m_booker->trade(trade_tick);

        if (!verification_passed) {
            m_exit_code++;
        }

        m_reporter->exchange_trade_tick_arrived(trade_tick);
    }
}

trade::types::OrderType trade::OfflineBooker::to_order_type(const char order_type)
{
    switch (order_type) {
    case 'L': return types::OrderType::limit;
    case 'M': return types::OrderType::market;
    case 'B': return types::OrderType::best_price;
    case 'C': return types::OrderType::cancel;
    case 'T': return types::OrderType::fill;
    default: return types::OrderType::invalid_order_type;
    }
}

trade::booker::OrderTickPtr trade::OfflineBooker::to_order_tick(StdTick* std_tick)
{
    if (std_tick->order_type != types::OrderType::limit
        && std_tick->order_type != types::OrderType::market
        && std_tick->order_type != types::OrderType::best_price
        && std_tick->order_type != types::OrderType::cancel)
        return nullptr;

    auto order_tick = std::make_shared<types::OrderTick>();

    order_tick->set_unique_id(std_tick->ask_unique_id + std_tick->bid_unique_id);
    order_tick->set_order_type(std_tick->order_type);
    order_tick->set_symbol(std_tick->symbol);
    order_tick->set_side(to_side(std_tick->ask_unique_id, std_tick->bid_unique_id));
    order_tick->set_price_1000x(static_cast<int64_t>(std_tick->price * 1000));
    order_tick->set_quantity(std_tick->quantity);
    order_tick->set_exchange_date(std_tick->date);
    order_tick->set_exchange_time(to_exchange_time(std_tick->time));

    return order_tick;
}

trade::booker::TradeTickPtr trade::OfflineBooker::to_trade_tick(StdTick* std_tick)
{
    if (std_tick->order_type != types::OrderType::fill)
        return nullptr;

    auto trade_tick = std::make_shared<types::TradeTick>();

    trade_tick->set_ask_unique_id(std_tick->ask_unique_id);
    trade_tick->set_bid_unique_id(std_tick->bid_unique_id);
    trade_tick->set_symbol(std_tick->symbol);
    trade_tick->set_exec_price_1000x(static_cast<int64_t>(std_tick->price * 1000));
    trade_tick->set_exec_quantity(std_tick->quantity);
    trade_tick->set_exchange_date(std_tick->date);
    trade_tick->set_exchange_time(to_exchange_time(std_tick->time));

    return trade_tick;
}

trade::types::SideType trade::OfflineBooker::to_side(const int64_t ask_unique_id, const int64_t bid_unique_id)
{
    /// Assert only one of them is 0.
    assert(ask_unique_id == 0 ^ bid_unique_id == 0);

    return ask_unique_id == 0 ? types::SideType::buy : types::SideType::sell;
}

int64_t trade::OfflineBooker::to_exchange_time(const int64_t time)
{
    /// time example: 20210701092500000.
    return time % 1000000000;
}

std::set<trade::OfflineBooker*> trade::OfflineBooker::m_instances;
