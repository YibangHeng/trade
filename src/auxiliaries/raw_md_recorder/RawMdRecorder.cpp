#include <csignal>
#include <filesystem>
#include <iostream>
#include <pcap.h>

#include "auxiliaries/raw_md_recorder/RawMdRecorder.h"
#include "info.h"
#include "libbooker/BookerCommonData.h"
#include "libbroker/CUTImpl/CUTCommonData.h"
#include "utilities/AddressHelper.hpp"
#include "utilities/NetworkHelper.hpp"
#include "utilities/UdpPayloadGetter.hpp"

trade::RawMdRecorder::RawMdRecorder(const int argc, char* argv[])
    : AppBase("raw_md_recorder")
{
    m_instances.emplace(this);
    m_is_running = argv_parse(argc, argv);
}

trade::RawMdRecorder::~RawMdRecorder()
{
    if (m_sse_tick_writer.is_open()) [[likely]] {
        m_sse_tick_writer.flush();
        m_sse_tick_writer.close();
        logger->info("Flushed and closed SSE tick writer");
    }

    if (m_sse_l2_snap_writer.is_open()) [[likely]] {
        m_sse_l2_snap_writer.flush();
        m_sse_l2_snap_writer.close();
        logger->info("Flushed and closed SSE l2 snap writer");
    }

    if (m_szse_order_writer.is_open()) [[likely]] {
        m_szse_order_writer.flush();
        m_szse_order_writer.close();
        logger->info("Flushed and closed SZSE order writer");
    }

    if (m_szse_trade_writer.is_open()) [[likely]] {
        m_szse_trade_writer.flush();
        m_szse_trade_writer.close();
        logger->info("Flushed and closed SZSE trade writer");
    }

    if (m_szse_l2_snap_writer.is_open()) [[likely]] {
        m_szse_l2_snap_writer.flush();
        m_szse_l2_snap_writer.close();
        logger->info("Flushed and closed SZSE l2 snap writer");
    }
}

int trade::RawMdRecorder::run()
{
    if (!m_is_running) {
        return m_exit_code;
    }

    std::vector<std::thread> threads;

    tick_receiver();

    logger->info("App exited with code {}", m_exit_code.load());

    return m_exit_code;
}

int trade::RawMdRecorder::stop(int signal)
{
    spdlog::info("App exiting since received signal {}", signal);

    m_is_running = false;

    exit(0);
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

    /// Output folder.
    desc.add_options()("output-folder,o", boost::program_options::value<std::string>()->default_value("./output"), "folder to store output files");

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

void trade::RawMdRecorder::tick_receiver()
{
    char errbuf[PCAP_ERRBUF_SIZE];
    const auto handle = pcap_fopen_offline(stdin, errbuf);

    if (handle == nullptr) {
        logger->error("Failed to read PCAP stream: {}", errbuf);
        return;
    }

    pcap_pkthdr header {};

    while (m_is_running && feof(stdin) == 0) {
        const u_char* packet = pcap_next(handle, &header);

        if (packet == nullptr)
            continue;

        const auto [payload, length] = utilities::UdpPayloadGetter()(packet);

        switch (length) {
        case broker::sse_hpf_tick_size: writer(payload, broker::sse_hpf_tick_type); break;
        case broker::sse_hpf_l2_snap_size: writer(payload, broker::sse_hpf_l2_snap_type); break;
        case broker::szse_hpf_order_tick_size: writer(payload, broker::szse_hpf_order_tick_type); break;
        case broker::szse_hpf_trade_tick_size: writer(payload, broker::szse_hpf_trade_tick_type); break;
        case broker::szse_hpf_l2_snap_size: writer(payload, broker::szse_hpf_l2_snap_type); break;
        default: {
            logger->warn("Unexpected packet length: {}", header.len - 42);
            break;
        }
        }
    }
}

void trade::RawMdRecorder::writer(const u_char* packet, const uint8_t message_type)
{
    switch (message_type) {
    case broker::sse_hpf_tick_type: write_sse_tick(packet); break;
    case broker::sse_hpf_l2_snap_type: write_sse_l2_snap(packet); break;
    case broker::szse_hpf_order_tick_type: write_szse_order_tick(packet); break;
    case broker::szse_hpf_trade_tick_type: write_szse_trade_tick(packet); break;
    case broker::szse_hpf_l2_snap_type: write_szse_l2_snap(packet); break;
    default: break;
    }
}

void trade::RawMdRecorder::write_sse_tick(const u_char* packet)
{
    new_sse_tick_writer();

    const auto sse_tick = reinterpret_cast<const broker::SSEHpfTick*>(packet);

    m_sse_tick_writer << fmt::format(
        "{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{}\n",
        /// SSEHpfPackageHead.
        sse_tick->m_head.m_seq_num,
        sse_tick->m_head.m_msg_type,
        sse_tick->m_head.m_msg_len,
        sse_tick->m_head.m_exchange_id,
        sse_tick->m_head.m_data_year,
        sse_tick->m_head.m_data_month,
        sse_tick->m_head.m_data_day,
        sse_tick->m_head.m_send_time,
        sse_tick->m_head.m_category_id,
        sse_tick->m_head.m_msg_seq_id,
        sse_tick->m_head.m_seq_lost_flag,
        /// SSEHpfTick.
        sse_tick->m_tick_index,
        sse_tick->m_channel_id,
        sse_tick->m_symbol_id,
        sse_tick->m_secu_type,
        sse_tick->m_sub_secu_type,
        sse_tick->m_tick_time,
        sse_tick->m_tick_type,
        sse_tick->m_buy_order_no,
        sse_tick->m_sell_order_no,
        sse_tick->m_order_price,
        sse_tick->m_qty,
        sse_tick->m_trade_money,
        sse_tick->m_side_flag,
        sse_tick->m_instrument_status
    );
}

void trade::RawMdRecorder::write_sse_l2_snap(const u_char* packet)
{
    new_sse_l2_snap_writer();

    const auto sse_l2_snap = reinterpret_cast<const broker::SSEHpfL2Snap*>(packet);

    m_sse_l2_snap_writer << fmt::format(
        "{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{}\n",
        /// SSEHpfPackageHead.
        sse_l2_snap->m_head.m_seq_num,
        sse_l2_snap->m_head.m_msg_type,
        sse_l2_snap->m_head.m_msg_len,
        sse_l2_snap->m_head.m_exchange_id,
        sse_l2_snap->m_head.m_data_year,
        sse_l2_snap->m_head.m_data_month,
        sse_l2_snap->m_head.m_data_day,
        sse_l2_snap->m_head.m_send_time,
        sse_l2_snap->m_head.m_category_id,
        sse_l2_snap->m_head.m_msg_seq_id,
        sse_l2_snap->m_head.m_seq_lost_flag,
        /// SSEHpfL2Snap.
        sse_l2_snap->m_update_time,
        sse_l2_snap->m_symbol_id,
        sse_l2_snap->m_update_type,
        sse_l2_snap->m_secu_type,
        sse_l2_snap->m_prev_close,
        sse_l2_snap->m_open_price,
        sse_l2_snap->m_day_high,
        sse_l2_snap->m_day_low,
        sse_l2_snap->m_last_price,
        sse_l2_snap->m_close_price,
        sse_l2_snap->m_instrument_status,
        sse_l2_snap->m_trading_status,
        sse_l2_snap->m_trade_number,
        sse_l2_snap->m_trade_volume,
        sse_l2_snap->m_trade_value,
        sse_l2_snap->m_total_qty_bid,
        sse_l2_snap->m_weighted_avg_px_bid,
        sse_l2_snap->m_total_qty_ask,
        sse_l2_snap->m_weighted_avg_px_ask,
        sse_l2_snap->m_yield_to_maturity,
        sse_l2_snap->m_depth_bid,
        sse_l2_snap->m_depth_ask,
        sse_l2_snap->m_bid_px[0].m_px,
        sse_l2_snap->m_bid_px[0].m_qty,
        sse_l2_snap->m_bid_px[1].m_px,
        sse_l2_snap->m_bid_px[1].m_qty,
        sse_l2_snap->m_bid_px[2].m_px,
        sse_l2_snap->m_bid_px[2].m_qty,
        sse_l2_snap->m_bid_px[3].m_px,
        sse_l2_snap->m_bid_px[3].m_qty,
        sse_l2_snap->m_bid_px[4].m_px,
        sse_l2_snap->m_bid_px[4].m_qty,
        sse_l2_snap->m_bid_px[5].m_px,
        sse_l2_snap->m_bid_px[5].m_qty,
        sse_l2_snap->m_bid_px[6].m_px,
        sse_l2_snap->m_bid_px[6].m_qty,
        sse_l2_snap->m_bid_px[7].m_px,
        sse_l2_snap->m_bid_px[7].m_qty,
        sse_l2_snap->m_bid_px[8].m_px,
        sse_l2_snap->m_bid_px[8].m_qty,
        sse_l2_snap->m_bid_px[9].m_px,
        sse_l2_snap->m_bid_px[9].m_qty,
        sse_l2_snap->m_ask_px[0].m_px,
        sse_l2_snap->m_ask_px[0].m_qty,
        sse_l2_snap->m_ask_px[1].m_px,
        sse_l2_snap->m_ask_px[1].m_qty,
        sse_l2_snap->m_ask_px[2].m_px,
        sse_l2_snap->m_ask_px[2].m_qty,
        sse_l2_snap->m_ask_px[3].m_px,
        sse_l2_snap->m_ask_px[3].m_qty,
        sse_l2_snap->m_ask_px[4].m_px,
        sse_l2_snap->m_ask_px[4].m_qty,
        sse_l2_snap->m_ask_px[5].m_px,
        sse_l2_snap->m_ask_px[5].m_qty,
        sse_l2_snap->m_ask_px[6].m_px,
        sse_l2_snap->m_ask_px[6].m_qty,
        sse_l2_snap->m_ask_px[7].m_px,
        sse_l2_snap->m_ask_px[7].m_qty,
        sse_l2_snap->m_ask_px[8].m_px,
        sse_l2_snap->m_ask_px[8].m_qty,
        sse_l2_snap->m_ask_px[9].m_px,
        sse_l2_snap->m_ask_px[9].m_qty
    );
}

void trade::RawMdRecorder::write_szse_order_tick(const u_char* packet)
{
    new_szse_order_writer();

    const auto szse_order_tick = reinterpret_cast<const broker::SZSEHpfOrderTick*>(packet);

    m_szse_order_writer << fmt::format(
        "{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{}\n",
        /// SZSEHpfPackageHead.
        szse_order_tick->m_header.m_sequence,
        szse_order_tick->m_header.m_tick1,
        szse_order_tick->m_header.m_tick2,
        szse_order_tick->m_header.m_message_type,
        szse_order_tick->m_header.m_security_type,
        szse_order_tick->m_header.m_sub_security_type,
        szse_order_tick->m_header.m_symbol,
        szse_order_tick->m_header.m_exchange_id,
        szse_order_tick->m_header.m_quote_update_time,
        szse_order_tick->m_header.m_channel_num,
        szse_order_tick->m_header.m_sequence_num,
        szse_order_tick->m_header.m_md_stream_id,
        /// SZSEHpfOrderTick.
        szse_order_tick->m_px,
        szse_order_tick->m_qty,
        szse_order_tick->m_side,
        szse_order_tick->m_order_type
    );
}

void trade::RawMdRecorder::write_szse_trade_tick(const u_char* packet)
{
    new_szse_trade_writer();

    const auto szse_trade_tick = reinterpret_cast<const broker::SZSEHpfTradeTick*>(packet);

    m_szse_trade_writer << fmt::format(
        "{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{}\n",
        /// SZSEHpfPackageHead.
        szse_trade_tick->m_header.m_sequence,
        szse_trade_tick->m_header.m_tick1,
        szse_trade_tick->m_header.m_tick2,
        szse_trade_tick->m_header.m_message_type,
        szse_trade_tick->m_header.m_security_type,
        szse_trade_tick->m_header.m_sub_security_type,
        szse_trade_tick->m_header.m_symbol,
        szse_trade_tick->m_header.m_exchange_id,
        szse_trade_tick->m_header.m_quote_update_time,
        szse_trade_tick->m_header.m_channel_num,
        szse_trade_tick->m_header.m_sequence_num,
        szse_trade_tick->m_header.m_md_stream_id,
        /// SZSEHpfTradeTick.
        szse_trade_tick->m_bid_app_seq_num,
        szse_trade_tick->m_ask_app_seq_num,
        szse_trade_tick->m_exe_px,
        szse_trade_tick->m_exe_qty,
        szse_trade_tick->m_exe_type
    );
}

void trade::RawMdRecorder::write_szse_l2_snap(const u_char* packet)
{
    new_szse_l2_snap_writer();

    const auto szse_l2_snap = reinterpret_cast<const broker::SZSEHpfL2Snap*>(packet);

    m_szse_l2_snap_writer << fmt::format(
        "{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{}\n",
        /// SZSEHpfPackageHead.
        szse_l2_snap->m_header.m_sequence,
        szse_l2_snap->m_header.m_tick1,
        szse_l2_snap->m_header.m_tick2,
        szse_l2_snap->m_header.m_message_type,
        szse_l2_snap->m_header.m_security_type,
        szse_l2_snap->m_header.m_sub_security_type,
        szse_l2_snap->m_header.m_symbol,
        szse_l2_snap->m_header.m_exchange_id,
        szse_l2_snap->m_header.m_quote_update_time,
        szse_l2_snap->m_header.m_channel_num,
        szse_l2_snap->m_header.m_sequence_num,
        szse_l2_snap->m_header.m_md_stream_id,
        /// SZSEHpfL2Snap.
        szse_l2_snap->m_trading_phase_code,
        szse_l2_snap->m_trades_num,
        szse_l2_snap->m_total_quantity_trade,
        szse_l2_snap->m_total_value_trade,
        szse_l2_snap->m_previous_close_price,
        szse_l2_snap->m_last_price,
        szse_l2_snap->m_open_price,
        szse_l2_snap->m_day_high,
        szse_l2_snap->m_day_low,
        szse_l2_snap->m_today_close_price,
        szse_l2_snap->m_total_bid_weighted_avg_px,
        szse_l2_snap->m_total_bid_qty,
        szse_l2_snap->m_total_ask_weighted_avg_px,
        szse_l2_snap->m_total_ask_qty,
        szse_l2_snap->m_lpv,
        szse_l2_snap->m_iopv,
        szse_l2_snap->m_upper_limit_price,
        szse_l2_snap->m_lower_limit_price,
        szse_l2_snap->m_open_interest,
        szse_l2_snap->m_bid_unit[0].m_price,
        szse_l2_snap->m_bid_unit[0].m_qty,
        szse_l2_snap->m_bid_unit[1].m_price,
        szse_l2_snap->m_bid_unit[1].m_qty,
        szse_l2_snap->m_bid_unit[2].m_price,
        szse_l2_snap->m_bid_unit[2].m_qty,
        szse_l2_snap->m_bid_unit[3].m_price,
        szse_l2_snap->m_bid_unit[3].m_qty,
        szse_l2_snap->m_bid_unit[4].m_price,
        szse_l2_snap->m_bid_unit[4].m_qty,
        szse_l2_snap->m_bid_unit[5].m_price,
        szse_l2_snap->m_bid_unit[5].m_qty,
        szse_l2_snap->m_bid_unit[6].m_price,
        szse_l2_snap->m_bid_unit[6].m_qty,
        szse_l2_snap->m_bid_unit[7].m_price,
        szse_l2_snap->m_bid_unit[7].m_qty,
        szse_l2_snap->m_bid_unit[8].m_price,
        szse_l2_snap->m_bid_unit[8].m_qty,
        szse_l2_snap->m_bid_unit[9].m_price,
        szse_l2_snap->m_bid_unit[9].m_qty,
        szse_l2_snap->m_ask_unit[0].m_price,
        szse_l2_snap->m_ask_unit[0].m_qty,
        szse_l2_snap->m_ask_unit[1].m_price,
        szse_l2_snap->m_ask_unit[1].m_qty,
        szse_l2_snap->m_ask_unit[2].m_price,
        szse_l2_snap->m_ask_unit[2].m_qty,
        szse_l2_snap->m_ask_unit[3].m_price,
        szse_l2_snap->m_ask_unit[3].m_qty,
        szse_l2_snap->m_ask_unit[4].m_price,
        szse_l2_snap->m_ask_unit[4].m_qty,
        szse_l2_snap->m_ask_unit[5].m_price,
        szse_l2_snap->m_ask_unit[5].m_qty,
        szse_l2_snap->m_ask_unit[6].m_price,
        szse_l2_snap->m_ask_unit[6].m_qty,
        szse_l2_snap->m_ask_unit[7].m_price,
        szse_l2_snap->m_ask_unit[7].m_qty,
        szse_l2_snap->m_ask_unit[8].m_price,
        szse_l2_snap->m_ask_unit[8].m_qty,
        szse_l2_snap->m_ask_unit[9].m_price,
        szse_l2_snap->m_ask_unit[9].m_qty
    );
}

void trade::RawMdRecorder::new_sse_tick_writer()
{
    if (m_sse_tick_writer.is_open()) [[likely]]
        return;

    const std::filesystem::path file_path = fmt::format("{}/sse-tick.csv", m_arguments["output-folder"].as<std::string>());

    create_directories(std::filesystem::path(file_path).parent_path());

    m_sse_tick_writer.open(file_path);

    logger->info("Opened new SSE tick writer at {}", file_path.string());

    m_sse_tick_writer
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
        << "instrument_status"
        << std::endl;
}

void trade::RawMdRecorder::new_sse_l2_snap_writer()
{
    if (m_sse_l2_snap_writer.is_open()) [[likely]]
        return;

    const std::filesystem::path file_path = fmt::format("{}/sse-l2-snap.csv", m_arguments["output-folder"].as<std::string>());

    create_directories(std::filesystem::path(file_path).parent_path());

    m_sse_l2_snap_writer.open(file_path);

    logger->info("Opened new SSE l2 snap writer at {}", file_path.string());

    m_sse_l2_snap_writer
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
        /// SSEHpfL2Snap.
        << "update_time,"
        << "symbol_id,"
        << "secu_type,"
        << "update_type,"
        << "prev_close,"
        << "open_price,"
        << "day_high,"
        << "day_low,"
        << "last_price,"
        << "close_price,"
        << "instrument_status,"
        << "trading_status,"
        << "trade_number,"
        << "trade_volume,"
        << "trade_value,"
        << "total_qty_bid,"
        << "weighted_avg_px_bid,"
        << "total_qty_ask,"
        << "weighted_avg_px_ask,"
        << "yield_to_maturity,"
        << "depth_bid,"
        << "depth_ask,"
        << "bid_px_1,"
        << "bid_volume_1,"
        << "bid_px_2,"
        << "bid_volume_2,"
        << "bid_px_3,"
        << "bid_volume_3,"
        << "bid_px_4,"
        << "bid_volume_4,"
        << "bid_px_5,"
        << "bid_volume_5,"
        << "bid_px_6,"
        << "bid_volume_6,"
        << "bid_px_7,"
        << "bid_volume_7,"
        << "bid_px_8,"
        << "bid_volume_8,"
        << "bid_px_9,"
        << "bid_volume_9,"
        << "bid_px_10,"
        << "bid_volume_10,"
        << "ask_px_1,"
        << "ask_volume_1,"
        << "ask_px_2,"
        << "ask_volume_2,"
        << "ask_px_3,"
        << "ask_volume_3,"
        << "ask_px_4,"
        << "ask_volume_4,"
        << "ask_px_5,"
        << "ask_volume_5,"
        << "ask_px_6,"
        << "ask_volume_6,"
        << "ask_px_7,"
        << "ask_volume_7,"
        << "ask_px_8,"
        << "ask_volume_8,"
        << "ask_px_9,"
        << "ask_volume_9,"
        << "ask_px_10,"
        << "ask_volume_10"
        << std::endl;
}

void trade::RawMdRecorder::new_szse_order_writer()
{
    if (m_szse_order_writer.is_open()) [[likely]]
        return;

    const std::filesystem::path file_path = fmt::format("{}/szse-order-tick.csv", m_arguments["output-folder"].as<std::string>());

    create_directories(std::filesystem::path(file_path).parent_path());

    m_szse_order_writer.open(file_path);

    logger->info("Opened new SZSE order writer at {}", file_path.string());

    m_szse_order_writer
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
        << "order_type"
        << std::endl;
}

void trade::RawMdRecorder::new_szse_trade_writer()
{
    if (m_szse_trade_writer.is_open()) [[likely]]
        return;

    const std::filesystem::path file_path = fmt::format("{}/szse-trade-tick.csv", m_arguments["output-folder"].as<std::string>());

    create_directories(std::filesystem::path(file_path).parent_path());

    m_szse_trade_writer.open(file_path);

    logger->info("Opened new SZSE trade writer at {}", file_path.string());

    m_szse_trade_writer
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
        << "exe_type"
        << std::endl;
}

void trade::RawMdRecorder::new_szse_l2_snap_writer()
{
    if (m_szse_l2_snap_writer.is_open()) [[likely]]
        return;

    const std::filesystem::path file_path = fmt::format("{}/szse-l2-snap.csv", m_arguments["output-folder"].as<std::string>());

    create_directories(std::filesystem::path(file_path).parent_path());

    m_szse_l2_snap_writer.open(file_path);

    logger->info("Opened new SZSE l2 snap writer at {}", file_path.string());

    m_szse_l2_snap_writer
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
        /// SZSEL2Snap.
        << "trading_phase_code,"
        << "trades_num,"
        << "total_quantity_trade,"
        << "total_value_trade,"
        << "previous_close_price,"
        << "last_price,"
        << "open_price,"
        << "day_high,"
        << "day_low,"
        << "today_close_price,"
        << "total_bid_weighted_avg_px,"
        << "total_bid_qty,"
        << "total_ask_weighted_avg_px,"
        << "total_ask_qty,"
        << "lpv,"
        << "iopv,"
        << "upper_limit_price,"
        << "lower_limit_price,"
        << "open_interest,"
        << "bid_1_px,"
        << "bid_1_qty,"
        << "bid_2_px,"
        << "bid_2_qty,"
        << "bid_3_px,"
        << "bid_3_qty,"
        << "bid_4_px,"
        << "bid_4_qty,"
        << "bid_5_px,"
        << "bid_5_qty,"
        << "bid_6_px,"
        << "bid_6_qty,"
        << "bid_7_px,"
        << "bid_7_qty,"
        << "bid_8_px,"
        << "bid_8_qty,"
        << "bid_9_px,"
        << "bid_9_qty,"
        << "bid_10_px,"
        << "bid_10_qty,"
        << "ask_1_px,"
        << "ask_1_qty,"
        << "ask_2_px,"
        << "ask_2_qty,"
        << "ask_3_px,"
        << "ask_3_qty,"
        << "ask_4_px,"
        << "ask_4_qty,"
        << "ask_5_px,"
        << "ask_5_qty,"
        << "ask_6_px,"
        << "ask_6_qty,"
        << "ask_7_px,"
        << "ask_7_qty,"
        << "ask_8_px,"
        << "ask_8_qty,"
        << "ask_9_px,"
        << "ask_9_qty,"
        << "ask_10_px,"
        << "ask_10_qty"
        << std::endl;
}

std::set<trade::RawMdRecorder*> trade::RawMdRecorder::m_instances;
