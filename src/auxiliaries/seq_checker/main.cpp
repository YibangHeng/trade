#include <csignal>

#include "auxiliaries/seq_checker/SeqChecker.h"

auto main(const int argc, char* argv[]) -> int
{
    std::signal(SIGINT, trade::SeqChecker::signal);

    trade::SeqChecker seq_checker(argc, argv);
    return seq_checker.run();
}
