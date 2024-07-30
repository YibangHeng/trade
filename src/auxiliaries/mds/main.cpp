#include <csignal>

#include "auxiliaries/mds/OSTMdServer.h"

auto main(const int argc, char* argv[]) -> int
{
    std::signal(SIGINT, trade::OSTMdServer::signal);

    trade::OSTMdServer mds(argc, argv);
    return mds.run();
}
