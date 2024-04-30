#pragma once

#include <atomic>
#include <boost/program_options.hpp>
#include <set>

#include "AppBase.hpp"

namespace trade
{

class Trade: private boost::noncopyable, private AppBase
{
public:
    Trade(int argc, char* argv[]);
    ~Trade();

public:
    int run() const;
    int stop();
    static void signal(int signal);

private:
    bool argv_parse(int argc, char* argv[]);

private:
    boost::program_options::variables_map arguments;

private:
    std::atomic<bool> is_running = false;
    std::atomic<int> exit_code   = 0;

private:
    static std::set<Trade*> instances;
};

} // namespace trade
