#include <csignal>
#include <filesystem>
#include <iostream>
#include <muduo/base/Logging.h>

#include "auxiliaries/sub_reporter_client/SubReporterClient.h"
#include "info.h"
#include "libbooker/BookerCommonData.h"
#include "libbroker/CUTImpl/RawStructure.h"
#include "utilities/AddressHelper.hpp"
#include "utilities/NetworkHelper.hpp"
#include "utilities/TimeHelper.hpp"

trade::SubReporterClient::SubReporterClient(const int argc, char* argv[])
    : AppBase("raw_md_recorder"),
      m_codec([this]<typename ConnType, typename MessageType, typename TimestampType>(ConnType&& conn, MessageType&& message, TimestampType&& receive_time) { m_dispatcher.on_protobuf_message(std::forward<ConnType>(conn), std::forward<MessageType>(message), std::forward<TimestampType>(receive_time)); }),
      m_dispatcher([this]<typename ConnType, typename MessageType, typename TimestampType>(ConnType&& conn, MessageType&& message, TimestampType&& timestamp) { on_invalied_message(std::forward<ConnType>(conn), std::forward<MessageType>(message), std::forward<TimestampType>(timestamp)); })
{
    m_instances.emplace(this);
    m_is_running = argv_parse(argc, argv);
}

trade::SubReporterClient::~SubReporterClient()
{
    m_loop->quit();
    m_event_loop_future.wait();
}

int trade::SubReporterClient::run()
{
    if (!m_is_running) {
        return m_exit_code;
    }

    muduo::net::InetAddress address(
        m_arguments["address"].as<std::string>(),
        m_arguments["port"].as<uint16_t>()
    );

    muduo::Logger::setLogLevel(muduo::Logger::NUM_LOG_LEVELS);

    m_dispatcher.register_message_callback<types::NewSubscribeRsp>([this](const muduo::net::TcpConnectionPtr& conn, const utilities::MessagePtr& message, const muduo::Timestamp timestamp) { on_new_subscribe_rsp(conn, message, timestamp); });

    /// Start event loop.
    m_event_loop_future = std::async(std::launch::async, [this, address] {
        m_loop   = std::make_shared<muduo::net::EventLoop>();
        m_client = std::make_shared<muduo::net::TcpClient>(m_loop.get(), address, "SubReporterClient");

        m_client->setConnectionCallback([this](const muduo::net::TcpConnectionPtr& conn) { on_connected(conn); });
        m_client->setMessageCallback([this](const muduo::net::TcpConnectionPtr& conn, muduo::net::Buffer* buf, const muduo::Timestamp receive_time) { m_codec.on_message(conn, buf, receive_time); });

        m_client->connect();

        /// Enter event loop until quit.
        m_loop->loop();

        m_client.reset();
        m_loop.reset();
    });

    m_mutex.lock();

    logger->info("App exited with code {}", m_exit_code.load());

    return m_exit_code;
}

int trade::SubReporterClient::stop(int signal)
{
    spdlog::info("App exiting since received signal {}", signal);

    m_is_running = false;

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
    desc.add_options()("port,p", boost::program_options::value<uint16_t>()->default_value(10000), "remote server port");

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

std::set<trade::SubReporterClient*> trade::SubReporterClient::m_instances;
