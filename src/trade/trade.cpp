#include <csignal>
#include <iostream>
#include <thread>

#include "info.h"
#include "trade/trade.h"

trade::Trade::Trade(const int argc, char* argv[])
    : AppBase("trade")
{
    instances.emplace(this);
    is_running = argv_parse(argc, argv);
}

trade::Trade::~Trade() = default;

int trade::Trade::run() const
{
    if (!is_running) {
        return exit_code;
    }

    int64_t counter = 3;

    logger->info("Bomb has been planted");

    while (is_running) {
        logger->info("{} seconds to go", counter--);

        if (counter < 0) {
            std::raise(SIGINT);
        }

        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    return exit_code;
}

int trade::Trade::stop()
{
    is_running = false;
    return 0;
}

void trade::Trade::signal(const int signal)
{
    switch (signal) {
    case SIGINT:
    case SIGTERM: {
        for (const auto& instance : instances) {
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

    try {
        store(parse_command_line(argc, argv, desc), arguments);
    }
    catch (const boost::wrapexcept<boost::program_options::unknown_option>& e) {
        std::cout << e.what() << std::endl;
        exit_code = 1;
        return false;
    }

    notify(arguments);

    if (arguments.contains("help")) {
        std::cout << desc;
        return false;
    }

    if (arguments.contains("version")) {
        std::cout << fmt::format("trade {}", trade_VERSION) << std::endl;
        return false;
    }

    return true;
}

std::set<trade::Trade*> trade::Trade::instances;
