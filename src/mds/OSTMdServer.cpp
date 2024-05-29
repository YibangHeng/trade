#include <csignal>
#include <fast-cpp-csv-parser/csv.h>
#include <iostream>

#include "enums.pb.h"
#include "info.h"
#include "libbooker/BookerCommonData.h"
#include "mds/OSTMdServer.h"
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

    const auto buf = std::unique_ptr<char>(new char[sizeof(broker::SZSEHpfOrderTick)]);

    while (m_is_running) {
        const auto order_tick = emit_od_tick(m_arguments["od-files"].as<std::string>());

        memcpy(buf.get(), &order_tick, sizeof(broker::SZSEHpfOrderTick));
        server.send(std::string(buf.get(), sizeof(broker::SZSEHpfOrderTick)));

        logger->info("Emitted order tick: {:>6} {:>6.2f} {:>6} {:<4} {:>1}", order_tick.m_header.m_symbol, booker::BookerCommonData::to_price(static_cast<liquibook::book::Price>(order_tick.m_px)), booker::BookerCommonData::to_quantity(order_tick.m_qty), SideType_Name(booker::BookerCommonData::to_side(order_tick.m_side)), order_tick.m_order_type);

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
    desc.add_options()("od-files,o", boost::program_options::value<std::string>()->default_value("./data/cut-odtd/od.csv"), "Order ticks file");
    desc.add_options()("td-files,t", boost::program_options::value<std::string>()->default_value("./data/cut-odtd/od.csv"), "Trade ticks file");

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
        std::cout << fmt::format("mds {}", trade_VERSION) << std::endl;
        return false;
    }

    if (m_arguments.contains("debug")) {
        spdlog::set_level(spdlog::level::debug);
        this->logger->set_level(spdlog::level::debug);
    }

    return true;
}

const trade::broker::SZSEHpfOrderTick& trade::OSTMdServer::emit_od_tick(const std::string& path)
{
    static std::vector<broker::SZSEHpfOrderTick> order_ticks;

    if (order_ticks.empty()) {
        order_ticks = read_od(path);

        if (order_ticks.empty())
            throw std::runtime_error(fmt::format("No order ticks read from {}", path));
    }

    static size_t index = 0;

    if (!m_arguments.contains("enable-loop") && index > order_ticks.size() - 2)
        /// Stop emitting.
        m_is_running = false;

    return order_ticks[index++ % order_ticks.size()];
}

std::vector<trade::broker::SZSEHpfOrderTick> trade::OSTMdServer::read_od(const std::string& path)
{
    io::CSVReader<5> in(path);
    in.read_header(io::ignore_extra_column, "securityId", "price", "tradeQty", "side", "orderType");

    std::vector<broker::SZSEHpfOrderTick> order_ticks;
    order_ticks.reserve(in.get_file_line());

    std::string security_id;
    double price;
    int64_t trade_qty;
    char side;
    char order_type;

    while (in.read_row(security_id, price, trade_qty, side, order_type)) {
        order_ticks.emplace_back();

        auto& order_tick                   = order_ticks.back();

        order_tick.m_header.m_sequence     = ticker_taper();
        order_tick.m_header.m_message_type = broker::CUTCommonData::to_szse_datagram_type(types::X_OST_SZSEDatagramType::order);

        M_A {order_tick.m_header.m_symbol} = security_id;
        order_tick.m_px                    = booker::BookerCommonData::to_price(price);
        order_tick.m_qty                   = booker::BookerCommonData::to_quantity(trade_qty);
        order_tick.m_side                  = side;
        order_tick.m_order_type            = order_type;

        logger->debug("Load order tick: {:>6} {:>6.2f} {:>6} {:<4} {:>1}", order_tick.m_header.m_symbol, booker::BookerCommonData::to_price(static_cast<liquibook::book::Price>(order_tick.m_px)), booker::BookerCommonData::to_quantity(order_tick.m_qty), SideType_Name(booker::BookerCommonData::to_side(order_tick.m_side)), order_tick.m_order_type);
    }

    logger->debug("Loaded {} order ticks", order_ticks.size());

    return order_ticks;
}

std::set<trade::OSTMdServer*> trade::OSTMdServer::m_instances;
