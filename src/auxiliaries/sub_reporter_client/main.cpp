#include <csignal>

#include "auxiliaries/sub_reporter_client/SubReporterClient.h"

auto main(const int argc, char* argv[]) -> int
{
    std::signal(SIGINT, trade::SubReporterClient::signal);

    trade::SubReporterClient src(argc, argv);
    return src.run();
}
