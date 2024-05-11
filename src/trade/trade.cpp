#include <csignal>
#include <iostream>

#include "info.h"
#include "libbroker/CTPBroker.h"
#include "libholder/SQLiteHolder.h"
#include "libreporter/LogReporter.h"
#include "trade/trade.h"

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
        return 1;
    }

    auto reporter = std::make_shared<reporter::LogReporter>();

    auto holder   = std::make_shared<holder::SQLiteHolder>();

    std::shared_ptr<broker::IBroker> broker;

    if (config->get<std::string>("Broker.Type") == "CTP") {
        broker = std::make_shared<broker::CTPBroker>(
            config->get<std::string>("Broker.Config"),
            holder, 
            reporter
        );
    }
    else {
        logger->error("Unsupported broker type {}", config->get<std::string>("Broker.Type"));
        return 1;
    }

    broker->login();

    try {
        broker->wait_login();
    }
    catch (const std::runtime_error& e) {
        logger->error("Failed to login: {}", e.what());
        return 1;
    }

    /// TODO: Start networking event loop here.

    broker->logout();

    try {
        broker->wait_logout();
    }
    catch (const std::runtime_error& e) {
        logger->error("Failed to logout: {}", e.what());
        return 1;
    }

    return m_exit_code;
}

int trade::Trade::stop()
{
    m_is_running = false;
    return 0;
}

void trade::Trade::signal(const int signal)
{
    switch (signal) {
    case SIGINT:
    case SIGTERM: {
        for (const auto& instance : m_instances) {
            instance->stop();
        }

        spdlog::info("App exited since received signal {}", signal);

        break;
    }
    default: {
        spdlog::info("Signal {} omitted", signal);

        break;
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

    if (m_arguments.contains("help")) {
        std::cout << desc;
        return false;
    }

    if (m_arguments.contains("version")) {
        std::cout << fmt::format("trade {}", trade_VERSION) << std::endl;
        return false;
    }

    if (m_arguments.contains("debug")) {
        spdlog::set_level(spdlog::level::debug);
    }

    return true;
}

std::set<trade::Trade*> trade::Trade::m_instances;
