#include <csignal>
#include <filesystem>
#include <iostream>
#include <muduo/base/Logging.h>

#include "auxiliaries/sub_reporter_client/SubReporterClient.h"
#include "info.h"
#include "libbooker/BookerCommonData.h"
#include "utilities/AddressHelper.hpp"
#include "utilities/NetworkHelper.hpp"
#include "utilities/TimeHelper.hpp"
#include "utilities/ToJSON.hpp"

trade::SubReporterClient::SubReporterClient(const int argc, char* argv[])
    : AppBase("raw_md_recorder")
{
    m_instances.emplace(this);
    m_is_running = argv_parse(argc, argv);
}

int trade::SubReporterClient::run()
{
    if (!m_is_running) {
        return m_exit_code;
    }

    m_sub_reporter_client_impl = std::make_shared<SubReporterClientImpl>(
        m_arguments["address"].as<std::string>(),
        m_arguments["port"].as<uint16_t>(),
        [this](const muduo::net::TcpConnectionPtr& conn, const types::ExchangeL2Snap& exchange_l2_snap, const muduo::Timestamp timestamp) { on_l2_snap_arrived(conn, exchange_l2_snap, timestamp); },
        [this](const muduo::net::TcpConnectionPtr& conn, const types::NewSubscribeRsp& new_subscribe_rsp, const muduo::Timestamp timestamp) { on_new_subscribe_rsp(conn, new_subscribe_rsp, timestamp); },
        [this](const muduo::net::TcpConnectionPtr& conn) { on_connected(conn); },
        [this](const muduo::net::TcpConnectionPtr& conn) { on_disconnected(conn); }
    );

    m_sub_reporter_client_impl->start_logout();

    m_sub_reporter_client_impl->wait_logout();

    logger->info("App exited with code {}", m_exit_code.load());

    return m_exit_code;
}

int trade::SubReporterClient::stop(int signal)
{
    spdlog::info("App exiting since received signal {}", signal);

    m_is_running = false;

    m_sub_reporter_client_impl->notify_logout_success();

    return 0;
}

void trade::SubReporterClient::signal(const int signal)
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

bool trade::SubReporterClient::argv_parse(const int argc, char* argv[])
{
    boost::program_options::options_description desc("Allowed options");

    /// Help and version.
    desc.add_options()("help,h", "print help message");
    desc.add_options()("version,v", "print version string and exit");

    desc.add_options()("debug,d", "enable debug output");

    /// Remote address.
    desc.add_options()("address,a", boost::program_options::value<std::string>()->default_value("127.0.0.1"), "remote server address");
    desc.add_options()("port,p", boost::program_options::value<uint16_t>()->default_value(10100), "remote server port");

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

void trade::SubReporterClient::on_l2_snap_arrived(
    const muduo::net::TcpConnectionPtr& conn,
    const types::ExchangeL2Snap& exchange_tick_snap,
    muduo::Timestamp timestamp
) const
{
    logger->info("Received L2 tick {}", utilities::ToJSON()(exchange_tick_snap));
}

void trade::SubReporterClient::on_new_subscribe_rsp(
    const muduo::net::TcpConnectionPtr& conn,
    const types::NewSubscribeRsp& new_subscribe_rsp,
    muduo::Timestamp timestamp
) const
{
    logger->info("Subscribed to {}", fmt::join(new_subscribe_rsp.subscribed_symbols(), ", "));
}

void trade::SubReporterClient::on_connected(const muduo::net::TcpConnectionPtr& conn)
{
    if (conn->connected()) {
        logger->info("Connected to {}({})", conn->name(), conn->peerAddress().toIpPort());

        types::NewSubscribeReq req;

        req.set_request_id(ticker_taper());
        req.set_app_name(app_name());
        req.add_symbols_to_subscribe("*");

        utilities::ProtobufCodec::send(conn, req);
    }
    else {
        logger->error("Failed to connect to {}({})", conn->name(), conn->peerAddress().toIpPort());
    }
}

void trade::SubReporterClient::on_disconnected(const muduo::net::TcpConnectionPtr& conn) const
{
    logger->info("Disconnected from {}({})", conn->name(), conn->peerAddress().toIpPort());
}

std::set<trade::SubReporterClient*> trade::SubReporterClient::m_instances;
