#include <csignal>
#include <filesystem>
#include <iostream>

#include "info.h"
#include "libbooker/BookerCommonData.h"
#include "libbroker/CUTImpl/CUTCommonData.h"
#include "libbroker/CUTImpl/RawStructure.h"
#include "raw_md_recorder/RawMdRecorder.h"
#include "utilities/AddressHelper.hpp"
#include "utilities/MakeAssignable.hpp"
#include "utilities/NetworkHelper.hpp"
#include "utilities/TimeHelper.hpp"

trade::RawMdRecorder::RawMdRecorder(const int argc, char* argv[])
    : AppBase("RawMdRecorder")
{
    m_instances.emplace(this);
    m_is_running = argv_parse(argc, argv);
}

int trade::RawMdRecorder::run()
{
    if (!m_is_running) {
        return m_exit_code;
    }

    const auto addresses = m_arguments["addresses"].as<std::vector<std::string>>();

    std::vector<std::thread> threads;

    for (const auto& address : addresses)
        threads.emplace_back(&RawMdRecorder::odtd_receiver, this, address, m_arguments["interface-address"].as<std::string>());

    for (auto& thread : threads)
        thread.join();

    logger->info("App exited with code {}", m_exit_code.load());

    return m_exit_code;
}

int trade::RawMdRecorder::stop(int signal)
{
    spdlog::info("App exiting since received signal {}", signal);

    m_is_running = false;

    return 0;
}

void trade::RawMdRecorder::signal(const int signal)
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

bool trade::RawMdRecorder::argv_parse(const int argc, char* argv[])
{
    boost::program_options::options_description desc("Allowed options");

    /// Help and version.
    desc.add_options()("help,h", "print help message");
    desc.add_options()("version,v", "print version string and exit");

    desc.add_options()("debug,d", "enable debug output");

    /// Multicast addresses.
    const std::vector<std::string> default_addresses = {"239.255.255.255:5555"};
    desc.add_options()("addresses,a", boost::program_options::value<std::vector<std::string>>()->default_value(default_addresses, "239.255.255.255:5555")->multitoken(), "multicast addresses");
    desc.add_options()("interface-address,i", boost::program_options::value<std::string>()->default_value("0.0.0.0"), "local interface address");

    /// Output folder.
    desc.add_options()("output-folder,o", boost::program_options::value<std::string>()->default_value("./output"), "folder to store output files");

    try {
        store(parse_command_line(argc, argv, desc), m_arguments);
    }
    catch (const boost::wrapexcept<boost::program_options::unknown_option>& e) {
        std::cout << e.what() << std::endl;
        m_exit_code = 1;
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
        std::cout << fmt::format("raw_md_recorder {}", trade_VERSION) << std::endl;
        return false;
    }

    if (m_arguments.contains("debug")) {
        spdlog::set_level(spdlog::level::debug);
        this->logger->set_level(spdlog::level::debug);
    }

    return true;
}

void trade::RawMdRecorder::odtd_receiver(const std::string& address, const std::string& interface_address)
{
    const auto [multicast_ip, multicast_port] = utilities::AddressHelper::extract_address(address);
    utilities::MCClient<u_char[broker::max_udp_size]> client(multicast_ip, multicast_port, interface_address, true);

    logger->info("Joined multicast group {}:{} at {}", multicast_ip, multicast_port, interface_address);

    while (m_is_running) {
        const auto message = client.receive();

        /// Non-blocking receiver may return empty string.
        if (message.empty())
            continue;

        write(message, broker::CUTCommonData::get_exchange_type(message));
    }

    logger->info("Left multicast group {}:{} at {}", multicast_ip, multicast_port, interface_address);
}

void trade::RawMdRecorder::write(const std::string& message, const types::ExchangeType exchange_type)
{
    switch (exchange_type) {
    case types::ExchangeType::sse: {
        write_sse(message);
        break;
    }
    case types::ExchangeType::szse: {
        write_szse(message);
        break;
    }
    default: {
        logger->error("Invalid exchange type: {}", static_cast<int>(exchange_type));
        break;
    }
    }
}

void trade::RawMdRecorder::write_sse(const std::string& message)
{
    const auto order_tick = reinterpret_cast<const broker::SSEHpfTick*>(message.data());

    new_sse_writer(order_tick->m_symbol_id);

    m_order_writers[order_tick->m_symbol_id] << fmt::format(
        "{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{}\n",
        /// SSEHpfPackageHead.
        order_tick->m_head.m_seq_num,
        order_tick->m_head.m_msg_type,
        order_tick->m_head.m_msg_len,
        order_tick->m_head.m_exchange_id,
        order_tick->m_head.m_data_year,
        order_tick->m_head.m_data_month,
        order_tick->m_head.m_data_day,
        order_tick->m_head.m_send_time,
        order_tick->m_head.m_category_id,
        order_tick->m_head.m_msg_seq_id,
        order_tick->m_head.m_seq_lost_flag,
        /// SSEHpfTick.
        order_tick->m_tick_index,
        order_tick->m_channel_id,
        order_tick->m_symbol_id,
        order_tick->m_secu_type,
        order_tick->m_sub_secu_type,
        order_tick->m_tick_time,
        order_tick->m_tick_type,
        order_tick->m_buy_order_no,
        order_tick->m_sell_order_no,
        order_tick->m_order_price,
        order_tick->m_qty,
        order_tick->m_trade_money,
        order_tick->m_side_flag,
        order_tick->m_instrument_status,
        /// Time.
        utilities::Now<std::string>()()
    );
}

void trade::RawMdRecorder::write_szse(const std::string& message)
{
    const auto dategram_type = broker::CUTCommonData::to_szse_tick_type(reinterpret_cast<const broker::SZSEHpfPackageHead*>(message.data())->m_message_type);

    switch (dategram_type) {
    case types::X_OST_TickType::order: {
        const auto order_tick = reinterpret_cast<const broker::SZSEHpfOrderTick*>(message.data());

        new_szse_order_writer(order_tick->m_header.m_symbol);

        m_order_writers[order_tick->m_header.m_symbol] << fmt::format(
            "{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{}\n",
            /// SZSEHpfPackageHead.
            order_tick->m_header.m_sequence,
            order_tick->m_header.m_tick1,
            order_tick->m_header.m_tick2,
            order_tick->m_header.m_message_type,
            order_tick->m_header.m_security_type,
            order_tick->m_header.m_sub_security_type,
            order_tick->m_header.m_symbol,
            order_tick->m_header.m_exchange_id,
            order_tick->m_header.m_quote_update_time,
            order_tick->m_header.m_channel_num,
            order_tick->m_header.m_sequence_num,
            order_tick->m_header.m_md_stream_id,
            /// SZSEHpfOrderTick.
            order_tick->m_px,
            order_tick->m_qty,
            order_tick->m_side,
            order_tick->m_order_type,
            /// Time.
            utilities::Now<std::string>()()
        );

        break;
    }
    case types::X_OST_TickType::trade: {
        const auto trade_tick = reinterpret_cast<const broker::SZSEHpfTradeTick*>(message.data());

        new_szse_trade_writer(trade_tick->m_header.m_symbol);

        m_trade_writers[trade_tick->m_header.m_symbol] << fmt::format(
            "{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{}\n",
            /// SZSEHpfPackageHead.
            trade_tick->m_header.m_sequence,
            trade_tick->m_header.m_tick1,
            trade_tick->m_header.m_tick2,
            trade_tick->m_header.m_message_type,
            trade_tick->m_header.m_security_type,
            trade_tick->m_header.m_sub_security_type,
            trade_tick->m_header.m_symbol,
            trade_tick->m_header.m_exchange_id,
            trade_tick->m_header.m_quote_update_time,
            trade_tick->m_header.m_channel_num,
            trade_tick->m_header.m_sequence_num,
            trade_tick->m_header.m_md_stream_id,
            /// SZSEHpfTradeTick.
            trade_tick->m_bid_app_seq_num,
            trade_tick->m_ask_app_seq_num,
            trade_tick->m_exe_px,
            trade_tick->m_exe_qty,
            trade_tick->m_exe_type,
            /// Time.
            utilities::Now<std::string>()()
        );

        break;
    }
    default: break;
    }
}

void trade::RawMdRecorder::new_sse_writer(const std::string& symbol)
{
    if (!m_order_writers.contains(symbol)) [[unlikely]] {
        const std::filesystem::path file_path = fmt::format("{}/{}/{}.csv", m_arguments["output-folder"].as<std::string>(), "sse_order_and_trade", symbol);

        create_directories(std::filesystem::path(file_path).parent_path());
        m_order_writers.emplace(symbol, std::ofstream(file_path));

        logger->info("Opened new order writer at {}", file_path.string());

        m_order_writers[symbol]
            /// SSEHpfPackageHead.
            << "seq_num,"
            << "msg_type,"
            << "msg_len,"
            << "exchange_id,"
            << "data_year,"
            << "data_month,"
            << "data_day,"
            << "send_time,"
            << "category_id,"
            << "msg_seq_id,"
            << "seq_lost_flag,"
            /// SSEHpfOrderTick.
            << "tick_index,"
            << "channel_id,"
            << "symbol_id,"
            << "secu_type,"
            << "sub_secu_type,"
            << "tick_time,"
            << "tick_type,"
            << "buy_order_no,"
            << "sell_order_no,"
            << "order_price,"
            << "qty,"
            << "trade_money,"
            << "side_flag,"
            << "instrument_status,"
            /// Time.
            << "time"
            << std::endl;
    }
}

void trade::RawMdRecorder::new_szse_order_writer(const std::string& symbol)
{
    if (!m_order_writers.contains(symbol)) [[unlikely]] {
        const std::filesystem::path file_path = fmt::format("{}/{}/{}.csv", m_arguments["output-folder"].as<std::string>(), "szse_order", symbol);

        create_directories(std::filesystem::path(file_path).parent_path());
        m_order_writers.emplace(symbol, std::ofstream(file_path));

        logger->info("Opened new order writer at {}", file_path.string());

        m_order_writers[symbol]
            /// SZSEHpfPackageHead.
            << "sequence,"
            << "tick1,"
            << "tick2,"
            << "message_type,"
            << "security_type,"
            << "sub_security_type,"
            << "symbol,"
            << "exchange_id,"
            << "quote_update_time,"
            << "channel_num,"
            << "sequence_num,"
            << "md_stream_id,"
            /// SZSEHpfOrderTick.
            << "px,"
            << "qty,"
            << "side,"
            << "order_type,"
            /// Time.
            << "time"
            << std::endl;
    }
}

void trade::RawMdRecorder::new_szse_trade_writer(const std::string& symbol)
{
    if (!m_trade_writers.contains(symbol)) [[unlikely]] {
        const std::filesystem::path file_path = fmt::format("{}/{}/{}.csv", m_arguments["output-folder"].as<std::string>(), "szse_trade", symbol);

        create_directories(std::filesystem::path(file_path).parent_path());
        m_trade_writers.emplace(symbol, std::ofstream(file_path));

        logger->info("Opened new trade writer at {}", file_path.string());

        m_trade_writers[symbol]
            /// SZSEHpfPackageHead.
            << "sequence,"
            << "tick1,"
            << "tick2,"
            << "message_type,"
            << "security_type,"
            << "sub_security_type,"
            << "symbol,"
            << "exchange_id,"
            << "quote_update_time,"
            << "channel_num,"
            << "sequence_num,"
            << "md_stream_id,"
            /// SZSEHpfTradeTick.
            << "bid_app_seq_num,"
            << "ask_app_seq_num,"
            << "exe_px,"
            << "exe_qty,"
            << "exe_type,"
            /// Time.
            << "time"
            << std::endl;
    }
}

std::set<trade::RawMdRecorder*> trade::RawMdRecorder::m_instances;
