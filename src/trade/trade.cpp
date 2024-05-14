#include <csignal>
#include <iostream>

#include "info.h"
#include "libbroker/CTPBroker.h"
#include "libholder/SQLiteHolder.h"
#include "libreporter/LogReporter.h"
#include "trade/trade.h"
#include "utilities/Serializer.hpp"

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

    broker->start_login();

    try {
        broker->wait_login();
    }
    catch (const std::runtime_error& e) {
        logger->error("Failed to login: {}", e.what());
        return 1;
    }

    init_zmq(config->get<std::string>("Server.APIAddress", ""));

    m_exit_code = network_events();

    broker->start_logout();

    try {
        broker->wait_logout();
    }
    catch (const std::runtime_error& e) {
        logger->error("Failed to logout: {}", e.what());
        return 1;
    }

    logger->info("App exited with code {}", m_exit_code);

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

        spdlog::info("App exiting since received signal {}", signal);

        /// Send a notification to network event loop.
        void* context = zmq_ctx_new();
        void* socket  = ::zmq_socket(context, ZMQ_REQ);

        zmq_connect(socket, "tcp://localhost:10000");

        types::UnixSig unix_sig;
        unix_sig.set_sig(signal);

        const auto message_body = utilities::Serializer::serialize(types::MessageID::unix_sig, unix_sig);

        zmq_send(socket, message_body.c_str(), message_body.size(), ZMQ_DONTWAIT);

        zmq_close(socket);
        zmq_ctx_destroy(context);

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
        std::cout << fmt::format("trade {}", trade_VERSION) << std::endl;
        return false;
    }

    if (m_arguments.contains("debug")) {
        spdlog::set_level(spdlog::level::debug);
        this->logger->set_level(spdlog::level::debug);
    }

    return true;
}

void trade::Trade::init_zmq(const std::string& socket)
{
    if (socket.empty()) {
        logger->warn("API Server disabled since no socket provided");
        return;
    }

    zmq_context.reset(zmq_ctx_new());
    zmq_socket.reset(::zmq_socket(zmq_context.get(), ZMQ_REP));

    const auto code = zmq_bind(zmq_socket.get(), socket.c_str());

    if (code != 0) {
        throw std::runtime_error(fmt::format("Failed to bind ZMQ socket at {}: {}", socket, std::string(zmq_strerror(errno))));
    }

    logger->info("Bound ZMQ socket at {}", socket);
}

int trade::Trade::network_events() const
{
    zmq_msg_t zmq_request;

    while (true) {
        zmq_msg_init(&zmq_request);

        const auto code = zmq_msg_recv(&zmq_request, zmq_socket.get(), ZMQ_DONTWAIT);

        if (code < 0) {
            if (errno == EAGAIN)
                std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            else
                logger->error("Failed to receive ZMQ message: {}", std::string(zmq_strerror(errno)));

            continue;
        }

        /// TODO: Use base64 for message data here.
        logger->debug("Received ZMQ message  \"{}\"({} bytes)", zmq_msg_data(&zmq_request), zmq_msg_size(&zmq_request));

        /// TODO: Use std::string_view here.
        const auto [message_id, message_body] = utilities::Serializer::deserialize({static_cast<char*>(zmq_msg_data(&zmq_request)), zmq_msg_size(&zmq_request)});

        switch (message_id) {
        case types::MessageID::unix_sig: {
            types::UnixSig unix_sig;
            unix_sig.ParseFromString(message_body);

            if (m_is_running) {
                /// Make sure the unix_sig message is send by itself in @signal.
                /// Client has no promise to stop server.
                logger->warn("Unexpected signal {} received", unix_sig.sig());
            }
            else {
                logger->info("Network event loop exited since received signal {}", unix_sig.sig());
                zmq_msg_close(&zmq_request);
                return 0;
            }

            const auto rsp = utilities::Serializer::serialize(types::MessageID::unix_sig, unix_sig);

            zmq_send(zmq_socket.get(), rsp.data(), rsp.size(), ZMQ_DONTWAIT);

            break;
        }
        default: {
            /// TODO: Use base64 for message data here.
            logger->warn("Unknown Message ID {}({}) received with Message Body \"{}\"({} bytes)", MessageID_Name(message_id), static_cast<int>(message_id), message_body.data(), message_body.size());
        }
        }
    }
}

std::set<trade::Trade*> trade::Trade::m_instances;
