#include <csignal>

#include "raw_md_recorder/RawMdRecorder.h"

auto main(const int argc, char* argv[]) -> int
{
    std::signal(SIGINT, trade::RawMdRecorder::signal);

    trade::RawMdRecorder rmr(argc, argv);
    return rmr.run();
}
