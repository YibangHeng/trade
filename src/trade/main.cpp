#include <csignal>

#include "trade/trade.h"

auto main(const int argc, char* argv[]) -> int
{
    std::signal(SIGINT, trade::Trade::signal);

    trade::Trade trade(argc, argv);
    return trade.run();
}
