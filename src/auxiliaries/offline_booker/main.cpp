#include <csignal>

#include "auxiliaries/offline_booker/OfflineBooker.h"

auto main(const int argc, char* argv[]) -> int
{
    std::signal(SIGINT, trade::OfflineBooker::signal);

    trade::OfflineBooker offline_booker(argc, argv);
    return offline_booker.run();
}
