#pragma once

#include <atomic>
#include <boost/program_options.hpp>
#include <set>

#include "AppBase.hpp"

namespace trade
{

class Trade: private AppBase<>
{
public:
    Trade(int argc, char* argv[]);
    ~Trade() override;

public:
    int run();
    int stop();
    static void signal(int signal);

private:
    bool argv_parse(int argc, char* argv[]);

private:
    boost::program_options::variables_map m_arguments;

private:
    std::atomic<bool> m_is_running = false;
    std::atomic<int> m_exit_code   = 0;

private:
    static std::set<Trade*> m_instances;
};

} // namespace trade
