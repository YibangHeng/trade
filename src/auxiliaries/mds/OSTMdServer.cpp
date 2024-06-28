#include <csignal>
#include <fast-cpp-csv-parser/csv.h>
#include <iostream>

#include "enums.pb.h"
#include "info.h"
#include "libbooker/BookerCommonData.h"
#include "auxiliaries/mds/OSTMdServer.h"
#include "utilities/MakeAssignable.hpp"
#include "utilities/NetworkHelper.hpp"

trade::OSTMdServer::OSTMdServer(const int argc, char* argv[])
    : AppBase("mds")
{
    m_instances.emplace(this);
    m_is_running = argv_parse(argc, argv);
}

int trade::OSTMdServer::run()
{
    if (!m_is_running) {
        return m_exit_code;
    }

    utilities::MCServer server("239.255.255.255", 5555);

    std::vector<u_char> message_buffer;
    message_buffer.resize(sizeof(broker::SSEHpfTick));

    while (m_is_running) {
        const auto tick = emit_sse_tick(m_arguments["sse-tick-file"].as<std::string>());

        memcpy(message_buffer.data(), &tick, sizeof(broker::SSEHpfTick));
        server.send(message_buffer);

        logger->info("Emitted order tick: {:>6} {:>6} {:>6.2f} {:>6} {:>1} {:>1} {:>6}", tick.m_buy_order_no + tick.m_sell_order_no, tick.m_symbol_id, tick.m_order_price / 1000., tick.m_qty / 1000, tick.m_side_flag, tick.m_tick_type, tick.m_tick_time / 100);

        if (m_arguments["emit-interval"].as<int64_t>() > 0)
            std::this_thread::sleep_for(std::chrono::milliseconds(m_arguments["emit-interval"].as<int64_t>()));
    }

    logger->info("App exited with code {}", m_exit_code.load());

    return m_exit_code;
}

int trade::OSTMdServer::stop(int signal)
{
    spdlog::info("App exiting since received signal {}", signal);

    m_is_running = false;

    return 0;
}

void trade::OSTMdServer::signal(const int signal)
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

bool trade::OSTMdServer::argv_parse(const int argc, char* argv[])
{
    boost::program_options::options_description desc("Allowed options");

    /// Help and version.
    desc.add_options()("help,h", "print help message");
    desc.add_options()("version,v", "print version string and exit");

    desc.add_options()("debug,d", "enable debug output");

    /// Multicast address.
    desc.add_options()("address,a", boost::program_options::value<std::string>()->default_value("239.255.255.255"), "multicast address");
    desc.add_options()("port,p", boost::program_options::value<uint16_t>()->default_value(5555), "multicast port");

    /// Emit interval and loop mode.
    desc.add_options()("emit-interval,e", boost::program_options::value<int64_t>()->default_value(1000), "Emit interval in milliseconds");
    desc.add_options()("enable-loop,l", "Enable loop emitting");

    /// Order/Trade ticks files.
    desc.add_options()("sse-tick-file", boost::program_options::value<std::string>()->default_value("./data/sse-tick-file.csv"), "SSE tick file");

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

const trade::broker::SSEHpfTick& trade::OSTMdServer::emit_sse_tick(const std::string& path)
{
    static std::vector<broker::SSEHpfTick> order_ticks;

    if (order_ticks.empty()) {
        order_ticks = read_sse_tick(path);

        if (order_ticks.empty())
            throw std::runtime_error(fmt::format("No order ticks read from {}", path));
    }

    static size_t index = 0;

    if (!m_arguments.contains("enable-loop") && index > order_ticks.size() - 2)
        /// Stop emitting.
        m_is_running = false;

    return order_ticks[index++ % order_ticks.size()];
}

std::vector<trade::broker::SSEHpfTick> trade::OSTMdServer::read_sse_tick(const std::string& path) const
{
    io::CSVReader<25> in(path);

    in.read_header(
        io::ignore_extra_column,
        /// SSEHpfPackageHead.
        "seq_num",
        "msg_type",
        "msg_len",
        "exchange_id",
        "data_year",
        "data_month",
        "data_day",
        "send_time",
        "category_id",
        "msg_seq_id",
        "seq_lost_flag",
        /// SSEHpfTick.
        "tick_index",
        "channel_id",
        "symbol_id",
        "secu_type",
        "sub_secu_type",
        "tick_time",
        "tick_type",
        "buy_order_no",
        "sell_order_no",
        "order_price",
        "qty",
        "trade_money",
        "side_flag",
        "instrument_status"
    );

    std::vector<broker::SSEHpfTick> order_ticks;
    order_ticks.reserve(in.get_file_line());

    /// SSEHpfPackageHead.
    uint32_t m_seq_num;
    uint8_t m_msg_type;
    uint16_t m_msg_len;
    uint8_t m_exchange_id;
    uint16_t m_data_year;
    uint8_t m_data_month;
    uint8_t m_data_day;
    uint32_t m_send_time;
    uint8_t m_category_id;
    uint32_t m_msg_seq_id;
    uint8_t m_seq_lost_flag;
    /// SSEHpfTick.
    uint32_t m_tick_index;
    uint32_t m_channel_id;
    std::string m_symbol_id;
    uint8_t m_secu_type;
    uint8_t m_sub_secu_type;
    uint32_t m_tick_time;
    char m_tick_type;
    uint64_t m_buy_order_no;
    uint64_t m_sell_order_no;
    uint32_t m_order_price;
    uint64_t m_qty;
    uint64_t m_trade_money;
    char m_side_flag;
    uint8_t m_instrument_status;

    while (in.read_row(
        /// SSEHpfPackageHead.
        m_seq_num,
        m_msg_type,
        m_msg_len,
        m_exchange_id,
        m_data_year,
        m_data_month,
        m_data_day,
        m_send_time,
        m_category_id,
        m_msg_seq_id,
        m_seq_lost_flag,
        /// SSEHpfTick.
        m_tick_index,
        m_channel_id,
        m_symbol_id,
        m_secu_type,
        m_sub_secu_type,
        m_tick_time,
        m_tick_type,
        m_buy_order_no,
        m_sell_order_no,
        m_order_price,
        m_qty,
        m_trade_money,
        m_side_flag,
        m_instrument_status
    )) {
        order_ticks.emplace_back();

        auto& order_tick = order_ticks.back();

        /// SSEHpfPackageHead.
        order_tick.m_head.m_seq_num       = m_seq_num;
        order_tick.m_head.m_msg_type      = m_msg_type;
        order_tick.m_head.m_msg_len       = m_msg_len;
        order_tick.m_head.m_exchange_id   = m_exchange_id;
        order_tick.m_head.m_data_year     = m_data_year;
        order_tick.m_head.m_data_month    = m_data_month;
        order_tick.m_head.m_data_day      = m_data_day;
        order_tick.m_head.m_send_time     = m_send_time;
        order_tick.m_head.m_category_id   = m_category_id;
        order_tick.m_head.m_msg_seq_id    = m_msg_seq_id;
        order_tick.m_head.m_seq_lost_flag = m_seq_lost_flag;
        /// SSEHpfTick.
        order_tick.m_tick_index        = m_tick_index;
        order_tick.m_channel_id        = m_channel_id;
        M_A {order_tick.m_symbol_id}   = m_symbol_id;
        order_tick.m_secu_type         = m_secu_type;
        order_tick.m_sub_secu_type     = m_sub_secu_type;
        order_tick.m_tick_time         = m_tick_time;
        order_tick.m_tick_type         = m_tick_type;
        order_tick.m_buy_order_no      = m_buy_order_no;
        order_tick.m_sell_order_no     = m_sell_order_no;
        order_tick.m_order_price       = m_order_price;
        order_tick.m_qty               = m_qty;
        order_tick.m_trade_money       = m_trade_money;
        order_tick.m_side_flag         = m_side_flag;
        order_tick.m_instrument_status = m_instrument_status;
    }

    logger->debug("Loaded {} order ticks", order_ticks.size());

    return order_ticks;
}

std::set<trade::OSTMdServer*> trade::OSTMdServer::m_instances;
