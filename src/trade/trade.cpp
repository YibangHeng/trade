#include <csignal>
#include <fmt/ranges.h>
#include <iostream>

#include "info.h"
#include "libbroker/CTPBroker.h"
#include "libbroker/CUTBroker.h"
#include "libholder/SQLiteHolder.h"
#include "libreporter/AsyncReporter.h"
#include "libreporter/CSVReporter.h"
#include "libreporter/LogReporter.h"
#include "libreporter/MySQLReporter.h"
#include "libreporter/ShmReporter.h"
#include "libreporter/SubReporter.h"
#include "trade/trade.h"
#include "utilities/NetworkHelper.hpp"

trade::Trade::Trade(const int argc, char* argv[])
    : AppBase("trade")
{
    m_instances.emplace(this);
    m_is_running = argv_parse(argc, argv);
}

trade::Trade::~Trade() = default;

int trade::Trade::run()
{
    if (!m_is_running) {
        return m_exit_code;
    }

    /// Load config.
    try {
        reset_config(m_arguments["config"].as<std::string>());
    }
    catch (const boost::property_tree::ini_parser_error& e) {
        logger->error("Failed to load config: {}", e.what());
        return EXIT_FAILURE;
    }

    const auto log_reporter   = std::make_shared<reporter::LogReporter>();
    const auto mysql_reporter = std::make_shared<reporter::MySQLReporter>(
        config->get<std::string>("Output.MySQLUrl", ""),
        config->get<std::string>("Output.MySQLUser", ""),
        config->get<std::string>("Output.MySQLPassword", ""),
        config->get<std::string>("Output.MySQLDatabase", ""),
        config->get<std::string>("Output.MySQLTable", ""),
        log_reporter
    );
    const auto csv_reporter = std::make_shared<reporter::CSVReporter>(config->get<std::string>("Output.CSVOutputFolder"), mysql_reporter);
    const auto sub_reporter = std::make_shared<reporter::SubReporter>(10100, csv_reporter);
    const auto shm_reporter = std::make_shared<reporter::ShmReporter>(
        config->get<std::string>("Output.ShmName"),
        config->get<std::string>("Output.ShmMutexName"),
        config->get<size_t>("Output.ShmSize"),
        sub_reporter
    );
    const auto async_reporter = std::make_shared<reporter::AsyncReporter>(sub_reporter);

    /// Reporter.
    m_reporter = async_reporter;

    /// Holder.
    m_holder = std::make_shared<holder::SQLiteHolder>();

    if (config->get<std::string>("Broker.Type") == "CTP") {
        m_broker = std::make_shared<broker::CTPBroker>(
            config->get<std::string>("Broker.Config"),
            m_holder,
            m_reporter
        );
    }
    else if (config->get<std::string>("Broker.Type") == "CUT") {
        m_broker = std::make_shared<broker::CUTBroker>(
            config->get<std::string>("Broker.Config"),
            m_holder,
            m_reporter
        );
    }
    else {
        logger->error("Unsupported broker type {}", config->get<std::string>("Broker.Type"));
        return EXIT_FAILURE;
    }

    /// Trade login.
    if (config->get<bool>("Functionality.EnableTrade")) {
        m_broker->start_login();

        try {
            m_broker->wait_login();
        }
        catch (const std::runtime_error& e) {
            logger->error("Failed to login: {}", e.what());
            return EXIT_FAILURE;
        }
    }

    /// Market data subscription.
    if (config->get<bool>("Functionality.EnableMd")) {
        m_broker->subscribe({});
    }

    m_exit_code = network_events();

    /// Market data unsubscription.
    if (config->get<bool>("Functionality.EnableMd")) {
        m_broker->unsubscribe({});
    }

    /// Trade logout.
    if (config->get<bool>("Functionality.EnableTrade")) {
        m_broker->start_logout();

        try {
            m_broker->wait_logout();
        }
        catch (const std::runtime_error& e) {
            logger->error("Failed to logout: {}", e.what());
            return EXIT_FAILURE;
        }
    }

    logger->info("App exited with code {}", m_exit_code.load());

    return m_exit_code;
}

int trade::Trade::stop(int signal)
{
    spdlog::info("App exiting since received signal {}", signal);

    m_is_running = false;

    /// Don't block in interrupt handlers.
    std::thread notifier([this, signal] {
        /// Send signal to network event loop.
        auto listening_address = config->get<std::string>("Server.APIAddress");

        // Replace '*' in listening address with 'localhost'.
        const std::size_t pos = listening_address.find("*");
        if (pos != std::string::npos)
            listening_address.replace(pos, 1, "localhost");

        utilities::RRClient client(listening_address);

        types::UnixSig send_unix_sig;
        send_unix_sig.set_sig(signal);

        std::ignore = client.request<types::UnixSig>(types::MessageID::unix_sig, send_unix_sig);
    });

    notifier.detach();

    return 0;
}

void trade::Trade::signal(const int signal)
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

bool trade::Trade::argv_parse(const int argc, char* argv[])
{
    boost::program_options::options_description desc("Allowed options");

    /// Help and version.
    desc.add_options()("help,h", "print help message");
    desc.add_options()("version,v", "print version string and exit");

    desc.add_options()("debug,d", "enable debug output");

    /// Config file.
    desc.add_options()("config,c", boost::program_options::value<std::string>()->default_value("./etc/trade.ini"), "config file");

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

int trade::Trade::network_events() const
{
    utilities::RRServer server(config->get<std::string>("Server.APIAddress"));

    logger->info("Bound ZMQ socket at {}", config->get<std::string>("Server.APIAddress"));

    std::vector<u_char> message_buffer;

    while (m_is_running) {
        const auto [message_id, message_body_it] = server.receive(message_buffer);

        switch (message_id) {
        case types::MessageID::unix_sig: {
            types::UnixSig unix_sig;
            unix_sig.ParseFromArray(message_body_it.base(), static_cast<int>(message_buffer.size() - utilities::Serializer::HEAD_SIZE<>));

            if (m_is_running) {
                /// Make sure the unix_sig message is send by itself in @signal.
                /// Client has no promise to stop server.
                logger->warn("Unexpected signal {} received", unix_sig.sig());
            }
            else {
                server.send(types::MessageID::unix_sig, unix_sig);
                logger->info("Network event loop exited since received signal {}", unix_sig.sig());
                return 0;
            }

            server.send(types::MessageID::unix_sig, unix_sig);

            break;
        }
        case types::MessageID::new_order_req: {
            const auto new_order_req = std::make_shared<types::NewOrderReq>();
            new_order_req->ParseFromArray(message_body_it.base(), static_cast<int>(message_buffer.size()));

            const auto new_order_rsp = m_broker->new_order(new_order_req);

            server.send(types::MessageID::new_order_rsp, *new_order_rsp);

            break;
        }
        case types::MessageID::new_cancel_req: {
            const auto new_cancel_req = std::make_shared<types::NewCancelReq>();
            new_cancel_req->ParseFromArray(message_body_it.base(), static_cast<int>(message_buffer.size()));

            const auto new_cancel_rsp = m_broker->cancel_order(new_cancel_req);

            server.send(types::MessageID::new_cancel_rsp, *new_cancel_rsp);

            break;
        }
        case types::MessageID::new_cancel_all_req: {
            const auto new_cancel_all_req = std::make_shared<types::NewCancelAllReq>();
            new_cancel_all_req->ParseFromArray(message_body_it.base(), static_cast<int>(message_buffer.size()));

            const auto new_cancel_all_rsp = m_broker->cancel_all(new_cancel_all_req);

            server.send(types::MessageID::new_cancel_all_rsp, *new_cancel_all_rsp);

            break;
        }
        default: break;
        }
    }

    logger->info("Network event loop exited");

    return 0;
}

std::set<trade::Trade*> trade::Trade::m_instances;
