#include <csignal>
#include <iostream>

#include "info.h"
#include "libbooker/BookerCommonData.h"
#include "libbroker/CUTImpl/RawStructure.h"
#include "raw_md_recorder/RawMdRecorder.h"
#include "utilities/MakeAssignable.hpp"
#include "utilities/NetworkHelper.hpp"

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

    utilities::MCClient<u_char[broker::max_udp_size]> client("239.255.255.255", 5555, true);

    while (m_is_running) {
        const auto raw_message = client.receive();

        /// Non-blocking receiver may return empty string.
        if (raw_message.empty())
            continue;

        if (raw_message.size() < sizeof(broker::SZSEHpfPackageHead)) {
            logger->error("Wrong size of received data: {} bytes received, expected at least {} bytes", raw_message.size(), sizeof(broker::SZSEHpfPackageHead));
            continue;
        }

        write(raw_message);
    }

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

    /// Multicast address.
    desc.add_options()("address,a", boost::program_options::value<std::string>()->default_value("239.255.255.255"), "multicast address");
    desc.add_options()("port,p", boost::program_options::value<uint16_t>()->default_value(5555), "multicast port");

    /// Output folder.
    desc.add_options()("output-folder,o", boost::program_options::value<std::string>()->default_value("./output/"), "folder to store output files");

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

void trade::RawMdRecorder::write(const std::string& raw_message)
{
    const auto header = reinterpret_cast<const broker::SZSEHpfPackageHead*>(raw_message.data());

    /// 23 stands for 逐笔委托行情 and 24 stands for 逐笔成交行情.
    switch (header->m_message_type) {
    case 23: {
        const auto order_tick = reinterpret_cast<const broker::SZSEHpfOrderTick*>(raw_message.data());

        new_order_writer(order_tick->m_header.m_symbol);

        m_order_writers[order_tick->m_header.m_symbol]
            << order_tick->m_header.m_sequence
            << order_tick->m_header.m_tick1
            << order_tick->m_header.m_tick2
            << order_tick->m_header.m_message_type
            << order_tick->m_header.m_security_type
            << order_tick->m_header.m_sub_security_type
            << order_tick->m_header.m_symbol
            << order_tick->m_header.m_exchange_id
            << order_tick->m_header.m_quote_update_time
            << order_tick->m_header.m_channel_num
            << order_tick->m_header.m_sequence_num
            << order_tick->m_header.m_md_stream_id
            << std::endl;

        break;
    }
    case 24: {
        const auto trade_tick = reinterpret_cast<const broker::SZSEHpfTradeTick*>(raw_message.data());

        new_trade_writer(trade_tick->m_header.m_symbol);

        m_trade_writers[trade_tick->m_header.m_symbol]
            << trade_tick->m_header.m_sequence
            << trade_tick->m_header.m_tick1
            << trade_tick->m_header.m_tick2
            << trade_tick->m_header.m_message_type
            << trade_tick->m_header.m_security_type
            << trade_tick->m_header.m_sub_security_type
            << trade_tick->m_header.m_symbol
            << trade_tick->m_header.m_exchange_id
            << trade_tick->m_header.m_quote_update_time
            << trade_tick->m_header.m_channel_num
            << trade_tick->m_header.m_sequence_num
            << trade_tick->m_header.m_md_stream_id
            << trade_tick->m_bid_app_seq_num
            << trade_tick->m_ask_app_seq_num
            << trade_tick->m_exe_px
            << trade_tick->m_exe_qty
            << trade_tick->m_exe_type
            << std::endl;

        break;
    }
    default: break;
    }
}

void trade::RawMdRecorder::new_order_writer(const std::string& symbol)
{
    if (m_order_writers.contains(symbol)) [[unlikely]] {
        m_order_writers.emplace(symbol, std::ofstream(fmt::format("{}/{}/{}.csv", m_arguments["output-folder"].as<std::string>(), "order", symbol)));

        m_order_writers[symbol]
            /// SZSEHpfPackageHead.
            << "m_sequence,"
            << "m_tick1,"
            << "m_tick2,"
            << "m_message_type,"
            << "m_security_type,"
            << "m_sub_security_type,"
            << "m_symbol,"
            << "m_exchange_id,"
            << "m_quote_update_time,"
            << "m_channel_num,"
            << "m_sequence_num,"
            << "m_md_stream_id,"
            /// SZSEHpfOrderTick.
            << "m_px,"
            << "m_qty,"
            << "m_side,"
            << "m_order_type"
            << std::endl;
    }
}

void trade::RawMdRecorder::new_trade_writer(const std::string& symbol)
{
    if (m_trade_writers.contains(symbol)) [[unlikely]] {
        m_trade_writers.emplace(symbol, std::ofstream(fmt::format("{}/{}/{}.csv", m_arguments["output-folder"].as<std::string>(), "trade", symbol)));

        m_trade_writers[symbol]
            /// SZSEHpfPackageHead.
            << "m_sequence,"
            << "m_tick1,"
            << "m_tick2,"
            << "m_message_type,"
            << "m_security_type,"
            << "m_sub_security_type,"
            << "m_symbol,"
            << "m_exchange_id,"
            << "m_quote_update_time,"
            << "m_channel_num,"
            << "m_sequence_num,"
            << "m_md_stream_id,"
            /// SZSEHpfTradeTick.
            << "m_bid_app_seq_num,"
            << "m_ask_app_seq_num,"
            << "m_exe_px,"
            << "m_exe_qty,"
            << "m_exe_type"
            << std::endl;
    }
}

std::set<trade::RawMdRecorder*> trade::RawMdRecorder::m_instances;
