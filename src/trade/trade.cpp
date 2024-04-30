#include <csignal>
#include <thread>

#include "trade/trade.h"

trade::Trade::Trade(const int argc, char* argv[])
    : AppBase("trade")
{
    instances.emplace(this);
    is_running = true;
}

trade::Trade::~Trade() = default;

int trade::Trade::run() const
{
    int64_t counter = 3;

    logger->info("Bomb has been planted");

    while (is_running) {
        logger->info("{} seconds to go", counter--);

        if (counter < 0) {
            std::raise(SIGINT);
        }

        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    return 0;
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

std::set<trade::Trade*> trade::Trade::instances;
